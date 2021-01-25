#include "ResetCommandPool.h"

#include "../../utils/grjob.h"

namespace gr
{
namespace vkg
{

ResetCommandPool::ResetCommandPool() : mCommandSpaces(grjob::getNumThreads()), mDevice(nullptr)  {}

ResetCommandPool::ResetCommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device) : 
	mCommandSpaces(grjob::getNumThreads()), mDevice(device)
{
	vk::CommandPoolCreateInfo createInfo(
		flags | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		familyIdx);

	for (uint32_t i = 0; i < static_cast<uint32_t>(mCommandSpaces.size()); ++i) {
		mCommandSpaces[i] = { device.createCommandPool(createInfo), 0, {} };
	}

	mQueue = mDevice.getQueue(familyIdx, 0);
}

ResetCommandPool::CommandSpace& ResetCommandPool::getCS()
{
	return mCommandSpaces[grjob::getThreadId()];
}

const ResetCommandPool::CommandSpace& ResetCommandPool::getCS() const
{
	return mCommandSpaces[grjob::getThreadId()];
}

std::vector<vk::CommandBuffer> ResetCommandPool::newCommandBuffers(const uint32_t num)
{
	assert(num != 0);

	vk::CommandBufferAllocateInfo info = vk::CommandBufferAllocateInfo(
		getCS().pool,
		vk::CommandBufferLevel::ePrimary,
		num
	);

	std::vector<vk::CommandBuffer> cmds;

	CommandSpace& cs = getCS();
	
	if (cs.lastUsed + num >= static_cast<uint32_t>(cs.buffers.size())) {
		cmds = mDevice.allocateCommandBuffers(info);

		// store allocated information
		std::vector<vk::CommandBuffer>& buff = cs.buffers;
		buff.insert(end(buff), begin(cmds), end(cmds));
	}
	else {
		cmds.resize(num);
		for (uint32_t i = 0; i < num; ++i) {
			cmds[i] = cs.buffers[cs.lastUsed + i];
		}
		cs.lastUsed += num;
	}

	

	return cmds;
}

vk::CommandBuffer ResetCommandPool::newCommandBuffer()
{
	vk::CommandBufferAllocateInfo info = vk::CommandBufferAllocateInfo(
		getCS().pool,
		vk::CommandBufferLevel::ePrimary,
		1
	);

	CommandSpace& cs = getCS();


	vk::CommandBuffer cmd;
	if (cs.lastUsed + 1 >= static_cast<uint32_t>(cs.buffers.size())) {

		vk::Result res = mDevice.allocateCommandBuffers(&info, &cmd);
		if (res != vk::Result::eSuccess) {
			throw std::runtime_error("Error Allocate command buffers!!");
		}

		cs.buffers.push_back(cmd);
	}
	else {
		cmd = cs.buffers[cs.lastUsed++];
	}


	return cmd;
}


void ResetCommandPool::submitCommandBuffer(
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

bool ResetCommandPool::submitPresentationImage(
	const vk::SwapchainKHR swapChain,
	const uint32_t imageIdx,
	const vk::Semaphore* semaphoreToWait) const
{
	vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR(
		semaphoreToWait != nullptr,	// num semaphores to wait
		semaphoreToWait,				// semaphores to wait
		1,								// num of swap chains where to present
		&swapChain,						// swap chains
		&imageIdx,						// image idx for each swap chain
		nullptr							// Result info?
	);

	vk::Result res = mQueue.presentKHR(&presentInfo);

	return res == vk::Result::eSuccess;
}

void ResetCommandPool::reset(bool release)
{

	vk::CommandPoolResetFlags flags = release ?
		vk::CommandPoolResetFlagBits::eReleaseResources :
		vk::CommandPoolResetFlagBits{};

	for (CommandSpace& cs : mCommandSpaces) {
		mDevice.resetCommandPool(
			cs.pool,
			flags
		);

		cs.lastUsed = 0;
	}
}


void ResetCommandPool::destroy()
{
	for (CommandSpace& poolS : mCommandSpaces) {
		mDevice.destroyCommandPool(poolS.pool);
		poolS.pool = nullptr;
	}
}

ResetCommandPool::operator vk::CommandPool() const
{
	return getCS().pool;
}

vk::CommandPool ResetCommandPool::get() const
{
	return getCS().pool;
}

}; // namespace vkg
}; // namespace gr