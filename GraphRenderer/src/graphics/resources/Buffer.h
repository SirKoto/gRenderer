#pragma once

#include "Allocatable.h"

namespace gr
{
namespace vkg
{


class Buffer : public Allocatable
{
public:
	Buffer() = default;
	Buffer(vk::Buffer buffer, VmaAllocation alloc) : Allocatable(alloc), mBuffer(buffer) {}

	void setVkBuffer(vk::Buffer buff) { mBuffer = buff; }

	vk::Buffer getVkBuffer() const { return mBuffer; }

protected:

	vk::Buffer mBuffer;
};

}
}