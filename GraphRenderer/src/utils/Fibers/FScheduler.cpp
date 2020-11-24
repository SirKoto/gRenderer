#include "FScheduler.h"


#ifdef _WIN32
#define NOMINMAX 
#include <Windows.h>
#endif

#include <algorithm>

namespace gr
{

namespace grjob
{

FScheduler::FScheduler(uint32_t maxThreads) :
	mNumThreads(std::min(maxThreads, std::thread::hardware_concurrency()) - 1),
	mHighPriorityQueue(100), mMidPriorityQueue(100), mLowPriorityQueue(100), mMainThreadQueue(10),
	mExceptionFun(&s_defaultExceptionHande)
{
#ifdef _WIN32
	mMainThreadHandle = GetCurrentThread();
#endif
	if (mNumThreads)
	{
		mThreads = new std::thread[mNumThreads];
	}

	// Create main thread tokens
	FScheduler::sTls.tokens = std::make_unique<QueueTokens>(std::array< moodycamel::ConcurrentQueue<Task>*, 3>{&mHighPriorityQueue, & mMidPriorityQueue, & mLowPriorityQueue});
	FScheduler::sTls.scheduler = this;
	FScheduler::sTls.isMainThread = true;
}


FScheduler::~FScheduler()
{
	if (mThreads)
	{
		delete[] mThreads;
	}
}

void FScheduler::joinAllThreads() const
{
	for (std::thread* ptr = mThreads; ptr != mThreads + mNumThreads; ++ptr)
	{
		ptr->join();
	}
}

void FScheduler::startJobSystem()
{
	// create all fibers
	uint32_t i = 0;
	for (Fiber& fiber : mFibers) {
		size_t reservedStack = 0;
		if (i++ < NUM_LEAF_FIBERS)
		{
			reservedStack = 1ull << 16; // 64Kb
		}
		else {
			reservedStack = 1ull << 19; //512Kb
		}

		FiberContext* context = new FiberContext(i);

		fiber.create(reinterpret_cast<Fiber::FiberInitFun>(&s_funWorkerFiber), context, reservedStack);
	}

	for (std::atomic_flag& flag : mIsFiberUsedFlag)
	{
		flag.clear(std::memory_order_relaxed);
	}

	for (uint32_t i = 0; i < mNumThreads; ++i) {

		std::unique_ptr<QueueTokens> threadTokens = std::make_unique<QueueTokens>(
			std::array< moodycamel::ConcurrentQueue<Task>*, 3>{&mHighPriorityQueue, & mMidPriorityQueue, & mLowPriorityQueue}
		);

		mThreads[i] = std::thread(&s_funThread, this, std::move(threadTokens));
	}

	s_funThread(this, nullptr);

	joinAllThreads();

	for (Fiber& fiber : mFibers) {
		fiber.destroy();;
	}
}

void FScheduler::stopSystem()
{
	mStopExecution = true;
}

void FScheduler::setExceptionCatch(void(*function)(const std::exception&))
{
	mExceptionFun = function;
}

void FScheduler::scheduleJob(
	Priority priority,
	const Job& job,
	Counter** pCounter)
{
	scheduleJob(priority, job, pCounter, false);
}

void FScheduler::scheduleJob(Priority priority, const Job& job, Counter** pCounter, bool needsBigStack)
{
	// create/modify counter
	if (pCounter != nullptr)
	{
		if (*pCounter == nullptr) {
			*pCounter = new Counter(1);
		}
		else {
			(*pCounter)->increment(1);
		}
	}

	Task task{ job, (pCounter ? *pCounter : nullptr), needsBigStack };
	switch (priority)
	{
	case Priority::eHigh:
		FScheduler::sTls.scheduler->mHighPriorityQueue.enqueue(sTls.tokens->pHToken, task);
		break;
	case Priority::eMid:
		FScheduler::sTls.scheduler->mMidPriorityQueue.enqueue(sTls.tokens->pMToken, task);
		break;
	case Priority::eLow:
		FScheduler::sTls.scheduler->mLowPriorityQueue.enqueue(sTls.tokens->pLToken, task);
		break;
	default:
		assert(false);
	}
}

void FScheduler::scheduleBatch(Priority priority, const Job* jobs, uint32_t numJobs, Counter** pCounter)
{
	// create/modify counter
	if (pCounter != nullptr)
	{
		if (*pCounter == nullptr) {
			*pCounter = new Counter(1);
		}
		else {
			(*pCounter)->increment(1);
		}
	}

	Counter* const c = (pCounter ? *pCounter : nullptr);
	Task* tasks = new Task[numJobs];
	for (uint32_t i = 0; i < numJobs; ++i)
	{
		new (tasks + i) Task{jobs[i], c , false};
	}

	switch (priority)
	{
	case Priority::eHigh:
		FScheduler::sTls.scheduler->mHighPriorityQueue.enqueue_bulk(sTls.tokens->pHToken, tasks, numJobs);
		break;
	case Priority::eMid:
		FScheduler::sTls.scheduler->mMidPriorityQueue.enqueue_bulk(sTls.tokens->pHToken, tasks, numJobs);
		break;
	case Priority::eLow:
		FScheduler::sTls.scheduler->mLowPriorityQueue.enqueue_bulk(sTls.tokens->pHToken, tasks, numJobs);
		break;
	default:
		assert(false);
	}

	delete[] tasks;
}

void FScheduler::scheduleJobForMainThread(const Job& job, Counter** pCounter, bool needsBigStack)
{
	// create/modify counter
	if (pCounter != nullptr)
	{
		if (*pCounter == nullptr) {
			*pCounter = new Counter(1);
		}
		else {
			(*pCounter)->increment(1);
		}
	}

	Task task{ job, (pCounter ? *pCounter : nullptr), needsBigStack };

	FScheduler::sTls.scheduler->mMainThreadQueue.enqueue(task);
}

void FScheduler::waitForCounterAndFree(const Counter* counter, uint32_t value)
{
	waitForCounter(counter, value);

	delete counter;
}

void FScheduler::waitForCounter(const Counter* counter, uint32_t value)
{
	assert(counter != nullptr);
	FScheduler::sTls.tasksOnWait.push_back({ counter, value, FScheduler::sTls.currentFiber });

	// switch to main thread without setting the job finished flag
	FScheduler::sTls.scheduler->mFibers[FScheduler::sTls.currentFiber].switchTo(FScheduler::sTls.threadFiber);
}


bool FScheduler::tryGetHighPriorityNextTask(Task* task)
{

	if (FScheduler::sTls.isMainThread && mMainThreadQueue.try_dequeue(*task))
	{
		return true;
	}

	if (mHighPriorityQueue.try_dequeue(sTls.tokens->cHToken, *task))
	{
		return true;
	}

	return false;
}

bool FScheduler::tryGetNextTask(Task* task)
{

	if (mMidPriorityQueue.try_dequeue(sTls.tokens->cMToken, *task))
	{
		return true;
	}

	if (mLowPriorityQueue.try_dequeue(sTls.tokens->cLToken, *task))
	{
		return true;
	}


	return false;
}

FScheduler::FiberIdx FScheduler::acquireFiber(bool needsBigStack)
{
	for (FiberIdx i = (needsBigStack ? NUM_LEAF_FIBERS : 0); i < NUM_FIBERS; ++i)
	{
		if (!mIsFiberUsedFlag[i].test_and_set(std::memory_order_relaxed /*std::memory_order_acquire*/)) {
			return i;
		}
	}

	return NULL_FIBER;
}

void FScheduler::setThreadsAffinityToCore()
{
#ifdef _WIN32
	std::thread::native_handle_type handle;

	SetThreadAffinityMask(mMainThreadHandle, 1ull);

	uint32_t i = 1;
	for (std::thread* ptr = mThreads; ptr != mThreads + mNumThreads; ++ptr)
	{
		handle = ptr->native_handle();
		if (handle)
		{
			DWORD_PTR mask = 1ull << i++;
			SetThreadAffinityMask(handle, mask);
		}
	}
#endif
}

bool FScheduler::tryGetWaitingFiber(FiberIdx* fiber)
{
	for (std::list<WaitFiber>::const_iterator it = FScheduler::sTls.tasksOnWait.begin();
		it != FScheduler::sTls.tasksOnWait.end();
		++it) {
		if (it->value2resume == it->counter->getValue())
		{
			*fiber = it->fiber;
			FScheduler::sTls.tasksOnWait.erase(it);
			return true;
		}
	}

	return false;
}


void FScheduler::s_funWorkerFiber(const FiberContext* context)
{
	const FiberIdx idx = context->fiberIdx;
	const FScheduler* scheduler = FScheduler::sTls.scheduler;
	delete context;
	Job job;
	Counter* counterToDecrement = nullptr;
	// Double while loop to avoid recreating the try catch frame
	while (true) {
		try {
			while (true)
			{
				// copy job to avoid problems in change of fibers
				job = FScheduler::sTls.currentJob;
				counterToDecrement = FScheduler::sTls.counterToDecrement;

				job.run();

				FScheduler::sTls.fiberFinished = true;
				if (counterToDecrement != nullptr) {
					counterToDecrement->decrement(1);
				}

				scheduler->mFibers[idx].switchTo(FScheduler::sTls.threadFiber);
			}
		}
		catch (const std::exception& exc) {
			scheduler->mExceptionFun(exc);
		}
	}
}

thread_local FScheduler::TLS FScheduler::sTls;

void FScheduler::s_funThread(FScheduler* scheduler, std::unique_ptr<QueueTokens>&& tokens)
{
	if (tokens) {
		FScheduler::sTls.tokens = std::forward<std::unique_ptr<QueueTokens>>(tokens);
	}

	FScheduler::sTls.scheduler = scheduler;
	FScheduler::sTls.threadFiber.createFromCurrentThread();

	bool recievedTask = false;
	FiberIdx actualFiber = NULL_FIBER;
	Task actualTask;
	while (!scheduler->mStopExecution) {

		if (!recievedTask) {
			recievedTask = scheduler->tryGetHighPriorityNextTask(&actualTask);
		}

		if (!recievedTask) {
			recievedTask = tryGetWaitingFiber(&actualFiber);
		}

		if (!recievedTask) {
			recievedTask = scheduler->tryGetNextTask(&actualTask);
		}

		if (recievedTask && actualFiber == NULL_FIBER) {
			actualFiber = scheduler->acquireFiber(actualTask.needsBigStack);
		}

		// If no task was recieved, or no fiber is available wait and try again
		if (!recievedTask || actualFiber == NULL_FIBER) {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
			continue;
		}

		FScheduler::sTls.currentJob = actualTask.job;
		FScheduler::sTls.counterToDecrement = actualTask.counter;
		FScheduler::sTls.currentFiber = actualFiber;


		// Switch to selected fiber to complete the job
		FScheduler::sTls.threadFiber.switchTo(scheduler->mFibers[actualFiber]);

		if (FScheduler::sTls.fiberFinished) {
			scheduler->mIsFiberUsedFlag[actualFiber].clear(std::memory_order_relaxed /*std::memory_order_release*/);
			FScheduler::sTls.fiberFinished = false;
		}
		recievedTask = false;
		actualFiber = NULL_FIBER;
		actualTask = Task();
	}

}

void FScheduler::s_defaultExceptionHande(const std::exception& exc)
{
	FScheduler::sTls.scheduler->stopSystem();
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	std::cerr << "Exception triggered:\n\t" << exc.what() << std::endl;

	exit(1);
}

} // namespace grjob
} // namespace gr