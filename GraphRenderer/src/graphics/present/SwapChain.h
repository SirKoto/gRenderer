#pragma once
#define NOMINMAX
#include "../DeviceComp.h"
#include "../Window.h"

class SwapChain
{
public:
	SwapChain(const DeviceComp& device,
		const Window& window);

	void destroy(const vk::Device& device);

protected:
	vk::SwapchainKHR mSwapChain;

	std::vector<vk::Image> mImages;

	vk::Extent2D mExtent;
	vk::SurfaceFormatKHR mFormat;

};

