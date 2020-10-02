#include "CommandPool.h"

namespace gr
{
namespace vkg
{

CommandPool::CommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device)
{
	vk::CommandPoolCreateInfo createInfo(flags, familyIdx);

	mPool = device.createCommandPool(createInfo);
}

void CommandPool::destroy(vk::Device device)
{
	device.destroyCommandPool(mPool);
}

}; // namespace vkg
}; // namespace gr