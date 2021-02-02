#include "FreeCommandPool.h"

#include "../../utils/grjob.h"

namespace gr
{
namespace vkg
{


FreeCommandPool::FreeCommandPool() : mCommandSpace(grjob::getNumThreads()), mDevice(nullptr)
{
}

FreeCommandPool::FreeCommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device)
	: mCommandSpace(grjob::getNumThreads()), mDevice(device)
{
	vk::CommandPoolCreateInfo createInfo(
		flags,
		familyIdx);

	for (uint32_t i = 0; i < static_cast<uint32_t>(mCommandSpace.size()); ++i) {
		mCommandSpace[i].pool = device.createCommandPool(createInfo);
	}

	mQueue = mDevice.getQueue(familyIdx, 0);
}

FreeCommandPool& FreeCommandPool::operator=(const FreeCommandPool& o)
{
	this->mCommandSpace = o.mCommandSpace;
	this->mDevice = o.mDevice;
	this->mQueue = o.mQueue;

	return *this;
}

std::vector<FreeCommandPool::FreeCommandBuffer> gr::vkg::FreeCommandPool::newCommandBuffers(
	const uint32_t num)
{
	assert(num != 0);

	const uint32_t poolIdx = grjob::getThreadId();

	vk::CommandBufferAllocateInfo info = vk::CommandBufferAllocateInfo(
		mCommandSpace[poolIdx].pool,
		vk::CommandBufferLevel::ePrimary,
		num
	);

	std::vector<vk::CommandBuffer> cmds = mDevice.allocateCommandBuffers(info);

	std::vector<FreeCommandBuffer> freeCmds(num);
	for (uint32_t i = 0; i < num; ++i) {
		freeCmds[i] = { cmds[i], poolIdx };
	}

	return freeCmds;
}

FreeCommandPool::FreeCommandBuffer FreeCommandPool::newCommandBuffer()
{
	const uint32_t poolIdx = grjob::getThreadId();

	vk::CommandBufferAllocateInfo info = vk::CommandBufferAllocateInfo(
		mCommandSpace[poolIdx].pool,
		vk::CommandBufferLevel::ePrimary,
		1
	);

	FreeCommandBuffer fcb = { nullptr, poolIdx };
	vk::Result res = mDevice.allocateCommandBuffers(&info, &fcb.buffer);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error("Error: Can't allocate command buffer!");
	}

	return fcb;
}


void FreeCommandPool::flushFrees()
{
	for (CommandPoolSpace poolS : mCommandSpace) {
		mDevice.freeCommandBuffers(poolS.pool, poolS.toFree);
		poolS.toFree.clear();
	}
}

void FreeCommandPool::freeCommandBuffer(const FreeCommandBuffer& command)
{
	mFreeMutex.lock();
	mCommandSpace[command.id].toFree.push_back(command.buffer);
	mFreeMutex.unlock();
}

void FreeCommandPool::freeCommandBuffers(const std::vector<FreeCommandBuffer>& commands)
{
	mFreeMutex.lock();

	for (const FreeCommandBuffer& command : commands) {
		mCommandSpace[command.id].toFree.push_back(command.buffer);
	}

	mFreeMutex.unlock();

}


void FreeCommandPool::destroy()
{
	for (CommandPoolSpace poolS : mCommandSpace) {
		mDevice.destroyCommandPool(poolS.pool);
	}
	mCommandSpace.clear();
}

vk::CommandPool gr::vkg::FreeCommandPool::get() const
{
	return mCommandSpace[grjob::getThreadId()].pool;
}
}
}
