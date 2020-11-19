#pragma once

#include "Fiber.h"

#include <thread>
#include <array>
#include <numeric>
#include <memory>
#include <concurrentqueue/concurrentqueue.h>

#include "Job.h"

namespace gr
{

class FScheduler
{
public:

	FScheduler(uint32_t maxThreads = std::thread::hardware_concurrency());

	~FScheduler();

	FScheduler& operator=(const FScheduler&) = delete;


	void joinAllThreads() const;

	void startJobSystem();


protected:

	const uint32_t mNumThreads;

	std::thread::native_handle_type mMainThreadHandle = nullptr;
	std::thread* mThreads = nullptr;

	typedef uint16_t FiberIdx;
	static const size_t NUM_FIBERS = 160;
	static const size_t LEAF_FIBERS = 128;
	static const FiberIdx NULL_FIBER = std::numeric_limits<FiberIdx>::max();
	std::array<Fiber, NUM_FIBERS> mFibers;

	moodycamel::ConcurrentQueue<Job> mHighPriorityQueue;
	moodycamel::ConcurrentQueue<Job> mMidPriorityQueue;
	moodycamel::ConcurrentQueue<Job> mLowPriorityQueue;


	bool mStopExecution = false;

	struct QueueTokens {
		moodycamel::ProducerToken pHToken;
		moodycamel::ProducerToken pMToken;
		moodycamel::ProducerToken pLToken;

		moodycamel::ConsumerToken cHToken;
		moodycamel::ConsumerToken cMToken;
		moodycamel::ConsumerToken cLToken;

		QueueTokens(const std::array< moodycamel::ConcurrentQueue<Job>*, 3>& queues) 
			: pHToken(*queues[0]), pMToken(*queues[1]), pLToken(*queues[2]),
			  cHToken(*queues[0]), cMToken(*queues[1]), cLToken(*queues[2])
		{}

	};

	static thread_local struct 
	{
		 Fiber threadFiber;
		 FiberIdx actualFiber;
		 std::unique_ptr<QueueTokens> tokens;
		 
	} sTls;

	void setThreadsAffinityToCore();


	static void s_funWorkerFiber(const FScheduler* scheduler);


	static void s_funThread(const FScheduler* scheduler, std::unique_ptr<QueueTokens>&& tokens);

};

} // namespace gr
