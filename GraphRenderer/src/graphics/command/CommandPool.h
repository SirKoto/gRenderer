#pragma once

#include <vulkan/vulkan.hpp>

namespace gr
{
namespace vkg
{
	class RenderContext;
	class CommandPool
	{
	public:
		CommandPool() = default;
		CommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device);

		// num must be different from zero
		std::vector<vk::CommandBuffer> createCommandBuffers(const uint32_t num) const;

		vk::CommandBuffer createCommandBuffer() const;

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

		explicit operator vk::CommandPool() const;

		vk::CommandPool get() const;

		bool operator<(CommandPool const& o) const
		{
			return mDevice < o.mDevice && mPools < o.mPools;
		}

	private:
		std::vector<vk::CommandPool> mPools;
		vk::Device mDevice;
		vk::Queue mQueue;
	};

}; // namespace vkg
}; // namespace gr

