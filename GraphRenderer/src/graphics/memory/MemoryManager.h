#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc/vk_mem_alloc.h>

namespace gr
{
namespace vkg
{
	class MemoryManager
	{
	public:

		MemoryManager() = default;

		MemoryManager(vk::Instance instance,
			vk::PhysicalDevice physicalDevice,
			vk::Device logicalDevice);


		void createImageAllocation(const vk::ImageCreateInfo& imageInfo,
			vk::MemoryPropertyFlags requiredProperties,
			vk::MemoryPropertyFlags preferredProperties,
			vk::Image* outImage, VmaAllocation* outAllocation,
			VmaAllocationInfo* outAllocInfo = nullptr) const;

		void createBufferAllocation(const vk::BufferCreateInfo& bufferInfo,
			vk::MemoryPropertyFlags requiredProperties,
			vk::MemoryPropertyFlags preferredProperties,
			vk::Buffer* outBuffer, VmaAllocation* outAllocation,
			VmaAllocationInfo* outAllocInfo = nullptr) const;

		void freeAllocation(VmaAllocation allocation) const;

		void getAllocationInfo(VmaAllocation allocation,
			vk::DeviceMemory* outDeviceMemory,
			vk::DeviceSize* outOffset,
			vk::DeviceSize* outSize) const;

		bool isMemoryMappable(VmaAllocation allocation) const;

		[[nodiscard]] void* mapMemory(VmaAllocation allocation) const;
		
		void unmapMemory(VmaAllocation allocation) const;

		void destroy();

	private:
		VmaAllocator mAllocator = {};
	};
}; // namespace vkg
}; // namespace gr
