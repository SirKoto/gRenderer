#pragma once

#include <vulkan/vulkan.hpp>

class CommandPool
{
public:

	CommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device);

	void destroy(vk::Device device);

private:
	vk::CommandPool mPool;
};

