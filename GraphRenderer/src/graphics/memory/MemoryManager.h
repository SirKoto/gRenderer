#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc/vk_mem_alloc.h>


class MemoryManager
{
public:
	MemoryManager(vk::Instance instance,
		vk::PhysicalDevice physicalDevice,
		vk::Device logicalDevice);

	~MemoryManager();

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

	void getAllocationInfo(const VmaAllocation& allocation,
		vk::DeviceMemory* outDeviceMemory,
		vk::DeviceSize* outOffset,
		vk::DeviceSize* outSize) const;



	void destroy();

private:
	VmaAllocator mAllocator;
};

