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
	Buffer(vk::Buffer buffer, VmaAllocation alloc, vk::DeviceSize bytesSize) : Allocatable(alloc), mBuffer(buffer), mBytesSize(bytesSize) {}

	void setVkBuffer(vk::Buffer buff) { mBuffer = buff; }

	const vk::Buffer& getVkBuffer() const { return mBuffer; }

	void setSize(vk::DeviceSize bytesSize) { mBytesSize = bytesSize; }
	vk::DeviceSize getSize() const { return mBytesSize; }

	operator bool() const { return mBuffer; }

	Buffer& operator=(std::nullptr_t) { mBuffer = nullptr; mBytesSize = 0; return *this; }

protected:

	vk::Buffer mBuffer;
	vk::DeviceSize mBytesSize;
};

}
}