#pragma once

#include <vulkan/vulkan.hpp>

namespace gr
{
namespace vkg
{

	class CommandPool
	{
	public:

		CommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device);

		void destroy(vk::Device device);

	private:
		vk::CommandPool mPool;
	};

}; // namespace vkg
}; // namespace gr

