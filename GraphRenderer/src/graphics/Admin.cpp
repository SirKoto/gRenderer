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

		return CommandPool(queueIdx, {}, mDevice);
	}

	void Admin::destroyCommandPool(CommandPool& pool)
	{
		pool.destroy(mDevice);
	}

	Image2D Admin::createDeviceImage2D(
		const vk::Extent2D& extent,
		uint32_t mipLevels,
		vk::SampleCountFlagBits numSamples,
		vk::Format format,
		vk::ImageTiling tiling,
		vk::ImageUsageFlags usage)
	{
		Image2D image;
		image.setExtent(extent);

		vk::ImageCreateInfo createInfo(
			{},
			vk::ImageType::e2D,
			format,
			vk::Extent3D(extent, 1),
			mipLevels,
			1,
			numSamples,
			tiling,
			usage);

		mMemManager.createImageAllocation(createInfo,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			&image.mImage,
			&image.mAllocation);

		return image;
	}

	void Admin::destroyImage(Image2D& image)
	{
		mMemManager.freeAllocation(image.getAllocation());
		mDevice.getVkDevice().destroyImage(image.getVkImage());
	}

	void Admin::destroy()
	{
		mMemManager.destroy();
		mDevice.destroy();
	}

}; // namespace vkg
}; // namespace gr