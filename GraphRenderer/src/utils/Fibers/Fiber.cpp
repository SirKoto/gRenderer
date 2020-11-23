#include "Fiber.h"

#ifdef _WIN32
#define NOMINMAX 
#include <Windows.h>
#else
static_assert(false, "Fibers not supported in this OS");
#endif 
#include <cassert>

namespace gr
{
namespace grjob
{
void Fiber::createFromCurrentThread()
{
	assert(mHandle == nullptr);

#ifdef _WIN32
	this->mHandle = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
#endif // _WIN32

}

void Fiber::switchTo(const Fiber& fib) const
{
	assert(fib.mHandle && this->mHandle && fib.mHandle != this->mHandle);
#ifdef _WIN32
	SwitchToFiber(fib.mHandle);
#endif // _WIN32

}

void Fiber::create(FiberInitFun fun, void* userData, size_t reservedStack)
{
#ifdef _WIN32
	mHandle = CreateFiberEx(0, reservedStack,
		FIBER_FLAG_FLOAT_SWITCH,
		fun, userData);
#endif // _WIN32

}

void Fiber::destroy()
{
#ifdef _WIN32
	DeleteFiber(mHandle);
#endif // _WIN32

	mHandle = nullptr;
}

} // namespace grjob
} // namespace gr
