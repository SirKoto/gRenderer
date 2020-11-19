#pragma once

namespace gr
{


class Fiber
{
public:
	Fiber() = default;

	
	void createFromCurrentThread();

	void switchTo(const Fiber& fib) const;

	typedef void(*FiberInitFun)(void*);
	void create(FiberInitFun fun, void* userData, size_t reservedStack = 0);

	void destroy();


protected:

	void* mHandle = nullptr;

};


} // namespace gr
