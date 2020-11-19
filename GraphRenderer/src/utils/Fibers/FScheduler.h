#pragma once

#include "Fiber.h"

#include <thread>
#include <array>
#include <numeric>

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

	bool mStopExecution = false;

	static thread_local struct 
	{
		 Fiber threadFiber;
		 FiberIdx actualFiber;
	} sTls;

	void setThreadsAffinityToCore();


	static void s_funWorkerFiber(const FScheduler* scheduler);
	static void s_funThread(const FScheduler* scheduler);

};

} // namespace gr
