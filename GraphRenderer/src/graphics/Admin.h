#pragma once

#include "memory/MemoryManager.h"
#include "DeviceComp.h"
#include "command/CommandPool.h"
#include "present/SwapChain.h"

#include "resources/Image2D.h"

namespace gr
{
namespace vkg
{
	class Admin
	{
	public:

		Admin() = default;

		Admin(DeviceComp&& device, MemoryManager&& memManager);

		const DeviceComp& getDeviceComp() const { return mDevice; }

		Image2D createDeviceImage2D(const vk::Extent2D& extent,
			uint32_t mipLevels,
			vk::SampleCountFlagBits numSamples,
			vk::Format format,
			vk::ImageTiling tiling,
			vk::ImageUsageFlags usage,
			vk::ImageAspectFlags ImageAspect = vk::ImageAspectFlagBits::eColor);

		void safeDestroyImage(Image2D& image);

		vk::Semaphore createSemaphore() const;
		void destroySemaphore(vk::Semaphore semaphore) const;

		void waitIdle() const;

		enum class CommandPoolTypes {
			eGraphic,
			ePresent,
			NUM
		};

		const CommandPool* getCommandPool(const CommandPoolTypes type) const;
		uint32_t getQueueFamilyIndex(const CommandPoolTypes type) const;

		void destroyRenderPass(const vk::RenderPass renderPass) const;

		void destroyFramebuffer(const vk::Framebuffer framebuffer) const;

		void destroy();

	private:
		DeviceComp mDevice;
		MemoryManager mMemManager;

		std::vector<CommandPool> mCommandPools;

		void createCommandPools();
		void destroyCommandPools();


	};

}; // namespace vkg

}; // namespace gr
