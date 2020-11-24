#include "Context.h"

#include "../utils/FsTools.h"

namespace gr
{
namespace vkg
{

	Context::Context(const DeviceComp& device, const MemoryManager& memManager) : mDevice(device), mMemManager(memManager)
	{
		createCommandPools();
	}


	Image2D Context::createDeviceImage2D(
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
			mDevice.getVkDevice().createImageView(ivCreateInfo);


		Image2D r_image;
		r_image.setExtent(extent);
		r_image.setImage(image);
		r_image.setAllocation(alloc);
		r_image.setImageView(imageView);

		return r_image;
	}

	void Context::safeDestroyImage(Image2D& image)
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

	vk::Semaphore Context::createSemaphore() const
	{
		vk::SemaphoreCreateInfo createInfo;
		
		return mDevice.getVkDevice().createSemaphore(createInfo);
	}

	void Context::destroy(vk::Semaphore semaphore) const
	{
		mDevice.getVkDevice().destroySemaphore(semaphore);
	}

	void Context::waitIdle() const
	{
		mDevice.getVkDevice().waitIdle();
	}

	const CommandPool* Context::getCommandPool(const CommandPoolTypes type) const
	{
		assert(type != CommandPoolTypes::NUM);

		return mCommandPools.data() + static_cast<size_t>(type);
	}

	uint32_t Context::getQueueFamilyIndex(const CommandPoolTypes type) const
	{
		switch (type)
		{
		case CommandPoolTypes::eGraphic:
			return mDevice.getGraphicsFamilyIdx();
		case CommandPoolTypes::ePresent:
			return mDevice.getPresentFamilyIdx();
		default:
			assert(false);
		}

		return -1;
	}

	void Context::destroy(const vk::RenderPass renderPass) const
	{
		mDevice.getVkDevice().destroyRenderPass(renderPass);
	}

	void Context::destroy(const vk::Framebuffer framebuffer) const
	{
		mDevice.getVkDevice().destroyFramebuffer(framebuffer);
	}

	void Context::createShaderModule(const std::string& fileName, vk::ShaderModule* module) const
	{
		assert(module != nullptr);

		std::vector<char> file;
		tools::loadBinaryFile(fileName, &file);

		if (file.empty()) {
			throw std::domain_error("Trying to run empty shader SPIR-V");
		}

		vk::ShaderModuleCreateInfo createInfo(
			{},			// flags
			file.size(),
			reinterpret_cast<uint32_t*>(file.data())
		);

		(*module) =  mDevice.getVkDevice().createShaderModule(createInfo);
	}

	void Context::destroy(const vk::ShaderModule module) const
	{
		mDevice.getVkDevice().destroyShaderModule(module);
	}


	void Context::destroy()
	{
		destroyCommandPools();

		mMemManager.destroy();
		mDevice.destroy();
	}

	void Context::createCommandPools()
	{

		mCommandPools.reserve(static_cast<size_t>(CommandPoolTypes::NUM));

		mCommandPools.push_back(CommandPool(mDevice.getGraphicsFamilyIdx(), {}, mDevice.getVkDevice()));


		if (mDevice.getPresentFamilyIdx() == mDevice.getGraphicsFamilyIdx())
		{
			mCommandPools.push_back(mCommandPools.back());
		}
		else {
			mCommandPools.push_back(CommandPool(mDevice.getPresentFamilyIdx(), {}, mDevice.getVkDevice()));
		}

	}

	void Context::destroyCommandPools()
	{

		mCommandPools[static_cast<size_t>(CommandPoolTypes::eGraphic)].destroy();
		if (mDevice.getPresentFamilyIdx() != mDevice.getGraphicsFamilyIdx()) {
			mCommandPools[static_cast<size_t>(CommandPoolTypes::ePresent)].destroy();
		}

	}

}; // namespace vkg
}; // namespace gr