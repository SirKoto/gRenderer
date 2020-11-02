#pragma once
#include <vulkan/vulkan.hpp>

#include "../DeviceComp.h"

namespace gr
{
namespace vkg
{

class RenderPass
{
public:

	vk::RenderPass getVkRenderPass() const { return mRenderPass; }

	operator vk::RenderPass() const { return mRenderPass; }

private:
	vk::RenderPass mRenderPass;

	friend class RenderPassBuilder;
};

}; // namespace vkg
}; // namespace gr