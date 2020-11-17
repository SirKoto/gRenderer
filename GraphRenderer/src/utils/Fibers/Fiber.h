#pragma once

namespace gr
{


class Fiber
{
public:
	Fiber() = default;

	
	void createFromCurrentThread();

	void create();

	void destroy();


protected:

	void* mHandle = nullptr;

};


} // namespace gr
