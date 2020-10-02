#pragma once

#include "memory/MemoryManager.h"
#include "DeviceComp.h"
#include "command/CommandPool.h"

#include "resources/Image2D.h"

namespace gr
{
namespace vkg
{
	class Admin
	{
	public:
		Admin(DeviceComp&& device, MemoryManager&& memManager);

		CommandPool createCommandPool(vk::QueueFlags queueType);
		void destroyCommandPool(CommandPool& pool);


		Image2D createDeviceImage2D(const vk::Extent2D& extent,
			uint32_t mipLevels,
			vk::SampleCountFlagBits numSamples,
			vk::Format format,
			vk::ImageTiling tiling,
			vk::ImageUsageFlags usage);

		void destroyImage(Image2D& image);


		void destroy();

	private:
		DeviceComp mDevice;
		MemoryManager mMemManager;
	};

}; // namespace vkg

}; // namespace gr
