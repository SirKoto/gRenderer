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

} // namespace gr