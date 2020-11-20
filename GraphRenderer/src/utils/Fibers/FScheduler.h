#pragma once

#include "Fiber.h"

#include <thread>
#include <array>
#include <numeric>
#include <memory>
#include <atomic>
#include <concurrentqueue/concurrentqueue.h>

#include "Job.h"

// Because of Windows....
#ifdef max
#undef max
#endif

namespace gr
{

class FScheduler
{
public:

	FScheduler(uint32_t maxThreads = std::thread::hardware_concurrency());

	~FScheduler();

	FScheduler& operator=(const FScheduler&) = delete;


	void startJobSystem();

	void stopSystem();


	enum class Priority {
		eHigh,
		eMid,
		eLow
	};


	// An object of this type needs to exist in order to use this functions
	static void scheduleJob(Priority priority, const Job& job);
	static void scheduleJob(Priority priority, bool needsBigStack, const Job& job);

	static void scheduleJobForMainThread(bool needsBigStack, const Job& job);



protected:

	const uint32_t mNumThreads;

	std::thread::native_handle_type mMainThreadHandle = nullptr;
	std::thread* mThreads = nullptr;

	typedef uint16_t FiberIdx;
	static const size_t NUM_FIBERS = 160;
	static const size_t NUM_LEAF_FIBERS = 128;
	static const FiberIdx NULL_FIBER = std::numeric_limits<FiberIdx>::max();
	std::array<Fiber, NUM_FIBERS> mFibers;
	std::array<std::atomic_flag, NUM_FIBERS> mIsFiberUsedFlag;

	typedef struct Task { 
		Job job;
		bool needsBigStack;

		Task() = default;
	} Task;


	// Allocated with 100 jobs at the begining each
	moodycamel::ConcurrentQueue<Task> mHighPriorityQueue;
	moodycamel::ConcurrentQueue<Task> mMidPriorityQueue;
	moodycamel::ConcurrentQueue<Task> mLowPriorityQueue;

	moodycamel::ConcurrentQueue<Task> mMainThreadQueue;


	bool mStopExecution = false;

	struct QueueTokens {
		moodycamel::ProducerToken pHToken;
		moodycamel::ProducerToken pMToken;
		moodycamel::ProducerToken pLToken;

		moodycamel::ConsumerToken cHToken;
		moodycamel::ConsumerToken cMToken;
		moodycamel::ConsumerToken cLToken;

		QueueTokens(const std::array< moodycamel::ConcurrentQueue<Task>*, 3>& queues)
			: pHToken(*queues[0]), pMToken(*queues[1]), pLToken(*queues[2]),
			  cHToken(*queues[0]), cMToken(*queues[1]), cLToken(*queues[2])
		{}

	};



	static thread_local struct 
	{
		FScheduler* scheduler;
		Fiber threadFiber;
		std::unique_ptr<QueueTokens> tokens;

		Task currentTask;

		std::list<Task> tasksOnWait;

		bool isMainThread = false;

	} sTls;

	void setThreadsAffinityToCore();

	bool tryGetNextTask(Task* task);

	void joinAllThreads() const;

	FiberIdx acquireFiber(bool needsBigStack = false);

	struct FiberContext
	{
		const FiberIdx fiberIdx;

		FiberContext(const FiberIdx fiberIdx) : fiberIdx(fiberIdx) {}
	};

	static void s_funWorkerFiber(const FiberContext* context);


	static void s_funThread(FScheduler* scheduler, std::unique_ptr<QueueTokens>&& tokens);

};

typedef FScheduler grjob;

} // namespace gr
