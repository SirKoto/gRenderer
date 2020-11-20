#include "FScheduler.h"


#ifdef _WIN32
#define NOMINMAX 
#include <Windows.h>
#endif

#include <algorithm>

namespace gr
{
FScheduler::FScheduler(uint32_t maxThreads) : 
	mNumThreads(std::min(maxThreads, std::thread::hardware_concurrency()) - 1),
	mHighPriorityQueue(100), mMidPriorityQueue(100), mLowPriorityQueue(100)
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

		FiberContext* context = new FiberContext(this, i);

		fiber.create(reinterpret_cast<Fiber::FiberInitFun>(&s_funWorkerFiber), context, reservedStack);
	}

	for (std::atomic_flag& flag : mIsFiberUsedFlag)
	{
		flag.clear(std::memory_order_relaxed);
	}

	for (uint32_t i = 0; i < mNumThreads; ++i) {

		std::unique_ptr<QueueTokens> threadTokens = std::make_unique<QueueTokens>(
			std::array< moodycamel::ConcurrentQueue<Task>*, 3>{&mHighPriorityQueue, &mMidPriorityQueue, &mLowPriorityQueue}
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

void FScheduler::scheduleJob(
	Priority priority,
	const Job& job)
{
	scheduleJob(priority, false, job);
}

void FScheduler::scheduleJob(Priority priority, bool needsBigStack, const Job& job)
{
	Task task{ job, needsBigStack };
	switch (priority)
	{
	case Priority::eHigh:
		mHighPriorityQueue.enqueue(sTls.tokens->pHToken, task);
		break;
	case Priority::eMid:
		mMidPriorityQueue.enqueue(sTls.tokens->pMToken, task);
		break;
	case Priority::eLow:
		mLowPriorityQueue.enqueue(sTls.tokens->pLToken, task);
		break;
	default:
		assert(false);
	}
}

bool FScheduler::tryGetNextTask(Task* task)
{

	if (mHighPriorityQueue.try_dequeue(sTls.tokens->cHToken, *task))
	{
		return true;
	}

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

void FScheduler::s_funWorkerFiber(const FiberContext* context)
{
	const FiberIdx idx = context->fiberIdx;
	const FScheduler* scheduler = context->scheduler;
	delete context;
	while (true)
	{
		Job& job = FScheduler::sTls.currentTask.job;

		job.run();

		scheduler->mFibers[idx].switchTo(FScheduler::sTls.threadFiber);
	}
}

void FScheduler::s_funThread(FScheduler* scheduler, std::unique_ptr<QueueTokens>&& tokens)
{
	if (tokens) {
		FScheduler::sTls.tokens = std::forward<std::unique_ptr<QueueTokens>>(tokens);
	}

	FScheduler::sTls.threadFiber.createFromCurrentThread();

	bool recievedTask = false;
	FiberIdx actualFiber = NULL_FIBER;
	while (!scheduler->mStopExecution) {
		if (!recievedTask) {
			recievedTask = scheduler->tryGetNextTask(&sTls.currentTask);
		}

		if (recievedTask) {
			actualFiber = scheduler->acquireFiber(FScheduler::sTls.currentTask.needsBigStack);
		}

		// If no task was recieved, or no fiber is available wait and try again
		if (!recievedTask || actualFiber == NULL_FIBER) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		// Switch to selected fiber to complete the job
		FScheduler::sTls.threadFiber.switchTo(scheduler->mFibers[actualFiber]);


		recievedTask = false;
		scheduler->mIsFiberUsedFlag[actualFiber].clear(std::memory_order_relaxed /*std::memory_order_release*/);
		actualFiber = NULL_FIBER;
	}

}

} // namespace gr