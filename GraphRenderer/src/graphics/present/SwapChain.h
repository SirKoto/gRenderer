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
		SwapChain(const DeviceComp& device,
			const Window& window);

		vk::Format getFormat() const { return mFormat.format; }
		vk::ColorSpaceKHR getColorSpace() const { return mFormat.colorSpace; }
		const vk::Extent2D& getExtent() const { return mExtent; }

		const std::vector<vk::Image>& getImagesVector() const { return mImages; }
		const std::vector<vk::ImageView>& getImageViewsVector() const { return mImageViews; }

		uint32_t getNumImages() const { return static_cast<uint32_t>(mImages.size()); }

		void destroy(const vk::Device& device);

	protected:
		vk::SwapchainKHR mSwapChain;

		std::vector<vk::Image> mImages;
		std::vector<vk::ImageView> mImageViews;

		vk::Extent2D mExtent;
		vk::SurfaceFormatKHR mFormat;
	};

}; // namespace vkg
}; // namespace gr


