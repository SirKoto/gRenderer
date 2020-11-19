#include "FScheduler.h"


#ifdef _WIN32
#define NOMINMAX 
#include <Windows.h>
#endif

#include <algorithm>

namespace gr
{
FScheduler::FScheduler(uint32_t maxThreads) : 
	mNumThreads(std::min(maxThreads, std::thread::hardware_concurrency()) - 1)
{
#ifdef _WIN32
	mMainThreadHandle = GetCurrentThread();
#endif
	mThreads = new std::thread[mNumThreads];
}


FScheduler::~FScheduler()
{
	if (mThreads)
	{
		delete mThreads;
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
		if (i++ < LEAF_FIBERS)
		{
			reservedStack = 1ull << 16; // 64Kb
		}
		else {
			reservedStack = 1ull << 19; //512Kb
		}


		fiber.create(reinterpret_cast<Fiber::FiberInitFun>(&s_funWorkerFiber), this, reservedStack);
	}

	for (uint32_t i = 0; i < mNumThreads; ++i) {
		mThreads[i] = std::thread(&s_funThread, this);
	}

	s_funThread(this);

	joinAllThreads();

	for (Fiber& fiber : mFibers) {
		fiber.destroy();;
	}
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

void FScheduler::s_funWorkerFiber(const FScheduler* scheduler)
{
	while (!scheduler->mStopExecution) {

	}

	FiberIdx idx = FScheduler::sTls.actualFiber;
	if (idx != NULL_FIBER)
	{
		scheduler->mFibers[idx].switchTo(FScheduler::sTls.threadFiber);
	}
}

void FScheduler::s_funThread(const FScheduler* scheduler)
{
	FScheduler::sTls.threadFiber.createFromCurrentThread();



}

} // namespace gr