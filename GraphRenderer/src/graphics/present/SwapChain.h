#pragma once
#define NOMINMAX
#include "../DeviceComp.h"
#include "../Window.h"

namespace gr
{
namespace vkg
{

	class SwapChain
	{
	public:

		SwapChain() = default;

		SwapChain(const DeviceComp& device,
			const Window& window);

		vk::Format getFormat() const { return mFormat.format; }
		vk::ColorSpaceKHR getColorSpace() const { return mFormat.colorSpace; }
		vk::PresentModeKHR getPresentMode() const { return mPresentMode; }

		vk::SurfaceFormatKHR getSurfaceFormat() const { return mFormat; }

		const vk::Extent2D& getExtent() const { return mExtent; }

		const std::vector<vk::Image>& getImagesVector() const { return mImages; }
		const std::vector<vk::ImageView>& getImageViewsVector() const { return mImageViews; }

		uint32_t getNumImages() const { return static_cast<uint32_t>(mImages.size()); }

		void recreateSwapChain(const DeviceComp& device,
			const Window& window);

		void destroy();

		const vk::SwapchainKHR &getVkSwapChain() const { return mSwapChain; }

		bool acquireNextImageBlock(const vk::Semaphore semaphore, uint32_t* imageIdx) const;

		std::vector<vk::Framebuffer> createFramebuffersOfSwapImages(const vk::RenderPass renderPass) const;


	protected:
		vk::SwapchainKHR mSwapChain;

		vk::Device mDevice;

		std::vector<vk::Image> mImages;
		std::vector<vk::ImageView> mImageViews;

		vk::Extent2D mExtent;
		vk::SurfaceFormatKHR mFormat;
		vk::PresentModeKHR mPresentMode;

		void createSwapChainAndImages(const DeviceComp& device, const Window& window);
	};

}; // namespace vkg
}; // namespace gr


