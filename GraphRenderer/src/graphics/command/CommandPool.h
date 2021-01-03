#pragma once

#include <vulkan/vulkan.hpp>

namespace gr
{
namespace vkg
{
	class Context;
	class CommandPool
	{
	public:

		CommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device);

		// num must be different from zero
		std::vector<vk::CommandBuffer> createCommandBuffers(const uint32_t num) const;

		void free(const vk::CommandBuffer* pCommandBuffers, uint32_t num) const;

		void submitCommandBuffer(const vk::CommandBuffer commandBuffer,
			const vk::Semaphore* waitSemaphore,
			const vk::PipelineStageFlags waitPipelineStage,
			const vk::Semaphore* signalSemaphore,
			vk::Fence fenceToSignal = nullptr) const;

		bool submitPresentationImage(
			const vk::SwapchainKHR swapChain,
			const uint32_t imageIdx,
			const vk::Semaphore* semaphoreToSignal) const;

		void destroy();

		explicit operator vk::CommandPool() const { return mPool; }

		vk::CommandPool get() const { return mPool;  }

		bool operator<(CommandPool const& o) const
		{
			return mDevice < o.mDevice && mPool < o.mPool;
		}

	private:
		vk::CommandPool mPool;
		vk::Device mDevice;
		vk::Queue mQueue;
	};

}; // namespace vkg
}; // namespace gr

