#include "Admin.h"


namespace gr
{
namespace vkg
{

	Admin::Admin(DeviceComp&& device, MemoryManager&& memManager) : mDevice(device), mMemManager(memManager)
	{
	}

	CommandPool Admin::createCommandPool(vk::QueueFlags queueType)
	{
		uint32_t queueIdx;
		if (queueType == vk::QueueFlagBits::eGraphics) {
			queueIdx = mDevice.getGraphicsFamilyIdx();
		}
		else if (queueType == vk::QueueFlagBits::eCompute) {
			queueIdx = mDevice.getComputeFamilyIdx();
		}
		else if (queueType == vk::QueueFlagBits::eTransfer) {
			queueIdx = mDevice.getTransferFamilyIdx();
		}
		else {
			throw std::invalid_argument("Wrong queue Family type");
		}

		return CommandPool(queueIdx, {}, mDevice.getDevice());
	}

	void Admin::destroyCommandPool(CommandPool& pool)
	{
		pool.destroy(mDevice.getDevice());
	}

	Image2D Admin::createDeviceImage2D(
		const vk::Extent2D& extent,
		uint32_t mipLevels,
		vk::SampleCountFlagBits numSamples,
		vk::Format format,
		vk::ImageTiling tiling,
		vk::ImageUsageFlags usage,
		vk::ImageAspectFlags imageAspect)
	{

		// Explicit sharing mode
		vk::ImageCreateInfo createInfo(
			{},							// flags
			vk::ImageType::e2D,			// Image Type
			format,						// Format
			vk::Extent3D(extent, 1),	// Extent
			mipLevels,					// Mip levels
			1,							// Array layers
			numSamples,					// SampleCount
			tiling,						// Image Tiling
			usage);						// Image usage

		vk::Image image;
		VmaAllocation alloc;

		mMemManager.createImageAllocation(createInfo,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			&image,
			&alloc);

		vk::ImageViewCreateInfo ivCreateInfo(
			{},						// flags
			image,					// image
			vk::ImageViewType::e2D, // image view type
			format,					// format
			{},						// no component mapping
			vk::ImageSubresourceRange(
				imageAspect,			// aspect
				0,						// base mip level
				mipLevels,				// level count
				0,						// base array layer
				1						// layer count
			)
		);
		vk::ImageView imageView =
			mDevice.getDevice().createImageView(ivCreateInfo);


		Image2D r_image;
		r_image.setExtent(extent);
		r_image.setImage(image);
		r_image.setAllocation(alloc);
		r_image.setImageView(imageView);

		return r_image;
	}

	void Admin::safeDestroyImage(Image2D& image)
	{
		if (image.getAllocation() != nullptr) {
			mMemManager.freeAllocation(image.getAllocation());
			image.setAllocation(nullptr);
		}
		if (static_cast<bool>(image.getVkImage())) {
			mDevice.getVkDevice().destroyImage(image.getVkImage());
			image.setImage(nullptr);
		}
		if (static_cast<bool>(image.getVkImageview())) {
			mDevice.getVkDevice().destroyImageView(image.getVkImageview());
			image.setImageView(nullptr);
		}
	}

	void Admin::destroy()
	{
		mMemManager.destroy();
		mDevice.destroy();
	}

}; // namespace vkg
}; // namespace gr