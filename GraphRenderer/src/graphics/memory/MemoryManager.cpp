#include "MemoryManager.h"

MemoryManager::MemoryManager(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device logicalDevice)
{
	VmaAllocatorCreateInfo createInfo = {};
	createInfo.instance = instance;
	createInfo.physicalDevice = physicalDevice;
	createInfo.device = logicalDevice;
	// TODO: lost allocations

	vmaCreateAllocator(&createInfo, &mAllocator);
}

MemoryManager::~MemoryManager()
{
}

void MemoryManager::destroy()
{
	vmaDestroyAllocator(mAllocator);
}
