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
		flags | vk::CommandPoolCreateFlagBits::eResetCommandBuffer
			  | vk::CommandPoolCreateFlagBits::eTransient,
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
	
	if (cs.nextToUseP + num - 1 >= static_cast<uint32_t>(cs.buffersP.size())) {
		cmds = mDevice.allocateCommandBuffers(info);

		// store allocated information
		std::vector<vk::CommandBuffer>& buff = cs.buffersP;
		buff.insert(end(buff), begin(cmds), end(cmds));
	}
	else {
		cmds.resize(num);
		for (uint32_t i = 0; i < num; ++i) {
			cmds[i] = cs.buffersP[cs.nextToUseP + i];
		}
		cs.nextToUseP += num;
	}

	

	return cmds;
}

vk::CommandBuffer ResetCommandPool::newCommandBuffer(vk::CommandBufferLevel level)
{
	vk::CommandBufferAllocateInfo info = vk::CommandBufferAllocateInfo(
		getCS().pool,
		level,
		1
	);

	vk::CommandBuffer cmd;

	// Lambda, to allocate the command buffer (primary or secondary)
	auto fun = [this, &cmd, &info](uint32_t* nextToUse, std::vector<vk::CommandBuffer>* buffers)
	{
		if (*nextToUse >= static_cast<uint32_t>(buffers->size())) {

			vk::Result res = mDevice.allocateCommandBuffers(&info, &cmd);
			if (res != vk::Result::eSuccess) {
				throw std::runtime_error("Error Allocate command buffers!!");
			}

			buffers->push_back(cmd);

			*nextToUse = static_cast<uint32_t>(buffers->size());
		}
		else {
			cmd = (*buffers)[(*nextToUse)++];
		}
	};

	CommandSpace& cs = getCS();

	if (level == vk::CommandBufferLevel::ePrimary) {
		fun(&cs.nextToUseP, &cs.buffersP);
	}
	else {
		fun(&cs.nextToUseS, &cs.buffersS);
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

		cs.nextToUseP = 0;
		cs.nextToUseS = 0;
	}
}


void ResetCommandPool::destroy()
{
	for (CommandSpace& poolS : mCommandSpaces) {
		mDevice.destroyCommandPool(poolS.pool);
		poolS.pool = nullptr;
	}

	mCommandSpaces.clear();
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