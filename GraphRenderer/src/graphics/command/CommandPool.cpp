#include "CommandPool.h"

#include "../../utils/grjob.h"

namespace gr
{
namespace vkg
{

CommandPool::CommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device) : 
	mPools(grjob::getNumThreads()), mDevice(device)
{
	vk::CommandPoolCreateInfo createInfo(flags, familyIdx);

	for (uint32_t i = 0; i < static_cast<uint32_t>(mPools.size()); ++i) {
		mPools[i] = device.createCommandPool(createInfo);
	}

	mQueue = mDevice.getQueue(familyIdx, 0);
}

std::vector<vk::CommandBuffer> CommandPool::createCommandBuffers(const uint32_t num) const
{
	assert(num != 0);

	vk::CommandBufferAllocateInfo info = vk::CommandBufferAllocateInfo(
		mPools[grjob::getThreadId()],
		vk::CommandBufferLevel::ePrimary,
		num
	);

	return mDevice.allocateCommandBuffers(info);
}

vk::CommandBuffer CommandPool::createCommandBuffer() const
{
	vk::CommandBufferAllocateInfo info = vk::CommandBufferAllocateInfo(
		mPools[grjob::getThreadId()],
		vk::CommandBufferLevel::ePrimary,
		1
	);
	vk::CommandBuffer cmd;
	vk::Result res = mDevice.allocateCommandBuffers(&info, &cmd);
	if (res != vk::Result::eSuccess) {
		throw std::runtime_error("Error Allocate command buffers!!");
	}

	return cmd;
}


void CommandPool::submitCommandBuffer(
	const vk::CommandBuffer commandBuffer,
	const vk::Semaphore* waitSemaphore,
	const vk::PipelineStageFlags waitPipelineStage,
	const vk::Semaphore* signalSemaphore,
	vk::Fence fenceToSignal) const
{
	vk::SubmitInfo submitInfo = vk::SubmitInfo(
		waitSemaphore != nullptr,		// Number of semaphores to wait on
		waitSemaphore,					// wait semaphore
		&waitPipelineStage,				// stage to block
		1,								// number of command buffers
		&commandBuffer,					// command buffer
		signalSemaphore != nullptr,		// number of semaphores to signal
		signalSemaphore					// signal semaphores
	);

	mQueue.submit({ submitInfo }, fenceToSignal);
}

bool CommandPool::submitPresentationImage(
	const vk::SwapchainKHR swapChain,
	const uint32_t imageIdx,
	const vk::Semaphore* semaphoreToSignal) const
{
	vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR(
		semaphoreToSignal != nullptr,	// num semaphores to wait
		semaphoreToSignal,				// semaphores to wait
		1,								// num of swap chains where to present
		&swapChain,						// swap chains
		&imageIdx,						// image idx for each swap chain
		nullptr							// Result info?
	);

	vk::Result res = mQueue.presentKHR(&presentInfo);

	return res == vk::Result::eSuccess;
}


void CommandPool::destroy()
{
	for (vk::CommandPool pool : mPools) {
		mDevice.destroyCommandPool(pool);
	}
}

CommandPool::operator vk::CommandPool() const
{
	return mPools[grjob::getThreadId()];
}

vk::CommandPool CommandPool::get() const
{
	return mPools[grjob::getThreadId()];
}

}; // namespace vkg
}; // namespace gr