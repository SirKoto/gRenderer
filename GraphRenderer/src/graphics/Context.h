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
	class Context
	{
	public:

		Context() = default;

		Context(const DeviceComp& device, const MemoryManager& memManager);

		const DeviceComp& getDeviceComp() const { return mDevice; }

		const vk::Device& getVkDevice() const { return mDevice.getVkDevice(); }

		Image2D createDeviceImage2D(const vk::Extent2D& extent,
			uint32_t mipLevels,
			vk::SampleCountFlagBits numSamples,
			vk::Format format,
			vk::ImageTiling tiling,
			vk::ImageUsageFlags usage,
			vk::ImageAspectFlags ImageAspect = vk::ImageAspectFlagBits::eColor);

		void safeDestroyImage(Image2D& image);

		vk::Semaphore createSemaphore() const;

		void createShaderModule(const char* fileName, vk::ShaderModule* module) const;

		void waitIdle() const;

		enum class CommandPoolTypes {
			eGraphic,
			ePresent,
			NUM
		};

		const CommandPool* getCommandPool(const CommandPoolTypes type) const;
		uint32_t getQueueFamilyIndex(const CommandPoolTypes type) const;

		void destroy(const vk::RenderPass renderPass) const;

		void destroy(const vk::Framebuffer framebuffer) const;

		void destroy(const vk::ShaderModule module) const;

		void destroy(vk::Semaphore semaphore) const;


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
