#pragma once

#include "../memory/MemoryManager.h"

namespace gr
{
namespace vkg
{


class Buffer
{
public:

	Buffer(vk::Buffer buffer, VmaAllocation alloc) : mBuffer(buffer), mAllocation(alloc) {}

	void setAllocation(VmaAllocation allocation) { mAllocation = allocation; }
	void setVkBuffer(vk::Buffer buff) { mBuffer = buff; }

	VmaAllocation getAllocation() const { return mAllocation; }
	vk::Buffer getVkBuffer() const { return mBuffer; }

protected:

	vk::Buffer mBuffer;
	VmaAllocation mAllocation;
};

}
}