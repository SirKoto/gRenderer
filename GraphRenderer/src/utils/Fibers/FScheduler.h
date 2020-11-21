#pragma once

#include "Fiber.h"

#include <thread>
#include <array>
#include <numeric>
#include <memory>
#include <atomic>
#include <concurrentqueue/concurrentqueue.h>

#include "Job.h"
#include "Counter.h"

// Because of Windows....
#ifdef max
#undef max
#endif

namespace gr
{

namespace grjob
{

enum class Priority {
	eHigh,
	eMid,
	eLow
};

class FScheduler
{
public:

	FScheduler(uint32_t maxThreads = std::thread::hardware_concurrency());

	~FScheduler();

	FScheduler& operator=(const FScheduler&) = delete;


	void startJobSystem();

	void stopSystem();


	// An object of this type needs to exist in order to use this functions
	static void scheduleJob(Priority priority, const Job& job, Counter** pCounter = nullptr);
	static void scheduleJob(Priority priority, bool needsBigStack, const Job& job, Counter** pCounter = nullptr);

	static void scheduleJobForMainThread(bool needsBigStack, const Job& job, Counter** pCounter = nullptr);

	static void waitForCounterAndFree(const Counter* counter, uint32_t value);
	static void waitForCounter(const Counter* counter, uint32_t value);


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
		Counter* counter = nullptr;
		bool needsBigStack = false;;

		Task() = default;
	} Task;

	typedef struct WaitFiber {
		const Counter* counter;
		const uint32_t value2resume;
		const FiberIdx fiber;
	} WaitFiber;


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


	struct TLS
	{
		FScheduler* scheduler = nullptr;
		Fiber threadFiber;

		std::unique_ptr<QueueTokens> tokens;

		Job currentJob;
		Counter* counterToDecrement = nullptr;

		std::list<WaitFiber> tasksOnWait;

		FiberIdx currentFiber = NULL_FIBER;
		bool fiberFinished = false;
		bool isMainThread = false;

		TLS() = default;

	};

	static thread_local TLS sTls;

	void setThreadsAffinityToCore();

	bool tryGetHighPriorityNextTask(Task* task);

	bool tryGetNextTask(Task* task);

	void joinAllThreads() const;

	FiberIdx acquireFiber(bool needsBigStack = false);

	struct FiberContext
	{
		const FiberIdx fiberIdx;

		FiberContext(const FiberIdx fiberIdx) : fiberIdx(fiberIdx) {}
	};

	static bool tryGetWaitingFiber(FiberIdx* fiber);

	static void s_funWorkerFiber(const FiberContext* context);


	static void s_funThread(FScheduler* scheduler, std::unique_ptr<QueueTokens>&& tokens);


};


	inline void scheduleJob(Priority priority, const Job& job, Counter** pCounter = nullptr) {
		FScheduler::scheduleJob(priority, job, pCounter);
	}
	inline void scheduleJob(Priority priority, bool needsBigStack, const Job& job, Counter** pCounter = nullptr) {
		FScheduler::scheduleJob(priority, needsBigStack, job, pCounter);
	}

	inline void scheduleJobForMainThread(bool needsBigStack, const Job& job, Counter** pCounter = nullptr) {
		FScheduler::scheduleJobForMainThread(needsBigStack, job, pCounter);
	}

	inline void waitForCounterAndFree(const Counter* counter, uint32_t value) {
		FScheduler::waitForCounterAndFree(counter, value);
	}

	inline void waitForCounter(const Counter* counter, uint32_t value) {
		FScheduler::waitForCounter(counter, value);
	}

} // namespace grjob
} // namespace gr
