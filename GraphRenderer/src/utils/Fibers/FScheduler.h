#pragma once

#include "Fiber.h"

#include <thread>
#include <array>

#include "../ConstExprHelp.h"

namespace gr
{

class FScheduler
{
public:

	FScheduler(uint32_t maxThreads = std::thread::hardware_concurrency());

	~FScheduler();

	FScheduler& operator=(const FScheduler&) = delete;


	void joinAllThreads() const;

protected:

	const uint32_t mNumThreads;

	std::thread::native_handle_type mMainThreadHandle = nullptr;
	std::thread* mThreads = nullptr;

	typedef uint16_t FiberIdx;
	static const size_t NUM_FIBERS = gr::cexprUtils::pow(8, sizeof(FiberIdx));
	std::array<Fiber, NUM_FIBERS> mFibers;

	

	static thread_local struct 
	{
		 Fiber threadFiber;
		 FiberIdx actualFiber;
	} mTls;

	void setThreadsAffinityToCore();


	static void FunWorkerThread();
	static void FunWorkerFiber();
	static void FunMainThread();

};

} // namespace gr
