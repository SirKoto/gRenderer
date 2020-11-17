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

void Fiber::createFromCurrentThread()
{
	assert(mHandle == nullptr);

#ifdef _WIN32
	this->mHandle = ConvertThreadToFiber(nullptr);
#endif // _WIN32

}

void Fiber::destroy()
{
#ifdef _WIN32
	DeleteFiber(mHandle);
#endif // _WIN32

	mHandle = nullptr;
}

} // namespace gr