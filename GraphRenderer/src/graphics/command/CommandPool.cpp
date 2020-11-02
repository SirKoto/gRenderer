#include "CommandPool.h"

namespace gr
{
namespace vkg
{

CommandPool::CommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device) : mDevice(device)
{
	vk::CommandPoolCreateInfo createInfo(flags, familyIdx);

	mPool = device.createCommandPool(createInfo);

	mQueue = mDevice.getQueue(familyIdx, 0);
}

std::vector<vk::CommandBuffer> CommandPool::createCommandBuffers(const uint32_t num) const
{
	assert(num != 0);

	vk::CommandBufferAllocateInfo info = vk::CommandBufferAllocateInfo(
		mPool,
		vk::CommandBufferLevel::ePrimary,
		num
	);

	return mDevice.allocateCommandBuffers(info);
}

void CommandPool::free(const vk::CommandBuffer* pCommandBuffers, uint32_t num) const
{
	mDevice.freeCommandBuffers(mPool, num, pCommandBuffers);
}

void CommandPool::submitCommandBuffer(
	const vk::CommandBuffer commandBuffer,
	const vk::Semaphore* waitSemaphore,
	const vk::PipelineStageFlags waitPipelineStage,
	const vk::Semaphore* signalSemaphore) const
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

	mQueue.submit(1, &submitInfo, nullptr);
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
	mDevice.destroyCommandPool(mPool);
}

}; // namespace vkg
}; // namespace gr