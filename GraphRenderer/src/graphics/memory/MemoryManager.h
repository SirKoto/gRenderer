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

	void destroy();
private:
	VmaAllocator mAllocator;
};

