#include "SwapChain.h"
#include <algorithm>
#include <iostream>

namespace gr
{
namespace vkg
{

SwapChain::SwapChain(const RenderContext& context, const Window& window) :  mDevice(context.getDevice())
{
	createSwapChainAndImages(context, window);
}

void SwapChain::createSwapChainAndImages(const RenderContext& context, const Window& window)
{
	vk::SurfaceKHR surface = window.getSurface();

	// Choose Surface format
	{
		std::vector<vk::SurfaceFormatKHR> availableFormats =
			static_cast<vk::PhysicalDevice>(context).getSurfaceFormatsKHR(surface);
		bool found = false;
		for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				mFormat = availableFormat;
				found = true;
				break;
			}
		}
		if (!found) {
			mFormat = *availableFormats.begin();
		}

	}
	// Choose present mode
	
	{
		std::vector<vk::PresentModeKHR> presentModes =
			static_cast<vk::PhysicalDevice>(context).getSurfacePresentModesKHR(surface);
		if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end()) {
			mPresentMode = vk::PresentModeKHR::eMailbox;
		}
		else {
			mPresentMode = vk::PresentModeKHR::eFifo;
		}
	}

	// Choose number of images that the swap chain can handle, and the images extent. Also select transform
	uint32_t numImages;
	vk::SurfaceTransformFlagBitsKHR preTransform;
	{
		vk::SurfaceCapabilitiesKHR capabilities =
			static_cast<vk::PhysicalDevice>(context).getSurfaceCapabilitiesKHR(surface);

		preTransform = capabilities.currentTransform;
		// Check if the capabilites automatically assigned extent, if not then assign it
		{
			if (capabilities.currentExtent.width != UINT32_MAX) {
				mExtent = capabilities.currentExtent;
			}
			else {
				mExtent = vk::Extent2D (window.getWidth(), window.getHeigth());

				mExtent.width =
					std::max(capabilities.minImageExtent.width,
						std::min(capabilities.maxImageExtent.width,
							mExtent.width));
				mExtent.height =
					std::max(capabilities.minImageExtent.height,
						std::min(capabilities.maxImageExtent.height,
							mExtent.height));
			}
		}
		// Select minimum number of images to be able to use vSync
		numImages = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 &&
			numImages > capabilities.maxImageCount) {
			numImages = capabilities.maxImageCount;
		}

		// Check that the swapChain supports images with dst_transfer operations
		if (!(capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst)) {
			throw std::runtime_error("Swap chain does not support transfer operations");
		}
	}

	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.surface = surface;
	createInfo.minImageCount = numImages;
	createInfo.imageFormat = mFormat.format;
	createInfo.imageColorSpace = mFormat.colorSpace;
	createInfo.presentMode = mPresentMode;
	createInfo.imageExtent = mExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	createInfo.preTransform = preTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.clipped = VK_TRUE;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;


	// Check if the graphics family and the present family are different
	if (context.getGraphicsFamilyIdx() != context.getPresentFamilyIdx()) {
		std::array<uint32_t, 2> indices = { context.getGraphicsFamilyIdx(),  context.getPresentFamilyIdx() };

		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(indices.size());
		createInfo.pQueueFamilyIndices = indices.data();
	}
	else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}

	mSwapChain = static_cast<vk::Device>(context).createSwapchainKHR(createInfo);

	mImages = static_cast<vk::Device>(context).getSwapchainImagesKHR(mSwapChain);


	// Create the image views
	mImageViews.resize(numImages);
	vk::ImageViewCreateInfo ivCreateInfo(
		{},						// flags
		nullptr,					// image
		vk::ImageViewType::e2D, // image view type
		mFormat.format,					// format
		{},						// no component mapping
		vk::ImageSubresourceRange(
			vk::ImageAspectFlagBits::eColor,// aspect
			0,						// base mip level
			1,						// level count
			0,						// base array layer
			1						// layer count
		)
	);
	for (uint32_t i = 0; i < numImages; ++i) {
		ivCreateInfo.image = mImages[i];
		mImageViews[i] = context.getDevice().createImageView(ivCreateInfo);
	}
}

void SwapChain::recreateSwapChain(const RenderContext& context, const Window& window)
{
	destroy();
	createSwapChainAndImages(context, window);
}

void SwapChain::destroy()
{
	for (vk::ImageView iv : mImageViews) {
		mDevice.destroyImageView(iv);
	}
	mDevice.destroySwapchainKHR(mSwapChain);
}

bool SwapChain::acquireNextImageBlock(const vk::Semaphore semaphore, uint32_t* imageIdx) const
{
	assert(imageIdx);

	vk::ResultValue<uint32_t> result = mDevice.acquireNextImageKHR(mSwapChain, UINT64_MAX, semaphore, nullptr);

	if (result.result != vk::Result::eSuccess) {
		if (result.result == vk::Result::eSuboptimalKHR) {
			std::cerr << ("Suboptimal SwapChain!! Needs to be recreated") << std::endl;
		}
	}

	*imageIdx = result.value;
	return result.result != vk::Result::eErrorOutOfDateKHR;
}

std::vector<vk::Framebuffer> SwapChain::createFramebuffersOfSwapImages(const vk::RenderPass renderPass) const
{
	vk::FramebufferCreateInfo createInfo = vk::FramebufferCreateInfo(
		{},			// flags
		renderPass,
		1, nullptr,
		mExtent.width,
		mExtent.height,
		1
	);

	const uint32_t size = static_cast<uint32_t>(mImages.size());
	
	std::vector<vk::Framebuffer> framebuffers(size);

	for (uint32_t i = 0; i < size; ++i) {
		createInfo.setPAttachments(mImageViews.data() + i);

		framebuffers[i] = mDevice.createFramebuffer(createInfo);
	}

	return framebuffers;
}


}; // namespace vkg
}; // namespace gr