#include "Context.h"

#include "../utils/FsTools.h"
#include "DebugVk.h"

namespace gr
{
namespace vkg
{

Context::Context(std::vector<const char*> extensions, bool loadGLFWextensions)
	: mInstance(extensions, loadGLFWextensions), mMemManager()
{ }

void Context::createDevice(bool enableAnisotropySampler,
	const vk::SurfaceKHR* surfaceToRequestSwapChain)
{
	mAnisotropySamplerEnabled = enableAnisotropySampler;
	mPresentQueueRequested = surfaceToRequestSwapChain != nullptr;
	pickAndCreatePysicalDevice(surfaceToRequestSwapChain);
	createLogicalDevice(surfaceToRequestSwapChain);
	createQueues();

	mMemManager = MemoryManager(mInstance, mPhysicalDevice, mDevice);

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
			getDevice().createImageView(ivCreateInfo);


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
			getDevice().destroyImage(image.getVkImage());
			image.setImage(nullptr);
		}
		if (static_cast<bool>(image.getVkImageview())) {
			getDevice().destroyImageView(image.getVkImageview());
			image.setImageView(nullptr);
		}
	}

	Buffer Context::createVertexBuffer(size_t sizeInBytes)
	{

		vk::BufferCreateInfo createInfo(
			vk::BufferCreateFlagBits(),	// flags
			sizeInBytes,				// size of buffer
			vk::BufferUsageFlagBits::eVertexBuffer,
			vk::SharingMode::eExclusive
		);

		vk::Buffer buffer;
		VmaAllocation alloc;

		mMemManager.createBufferAllocation(createInfo,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			&buffer,
			&alloc);


		return Buffer(buffer, alloc);
	}

	void Context::safeDestroyBuffer(Buffer& buffer)
	{
		if (buffer.getAllocation() != nullptr) {
			mMemManager.freeAllocation(buffer.getAllocation());
			buffer.setAllocation(nullptr);
		}
		if (static_cast<bool>(buffer.getVkBuffer())) {
			getDevice().destroyBuffer(buffer.getVkBuffer());
			buffer.setVkBuffer(nullptr);
		}
	}

	void Context::transferDataToGPU(const Allocatable& allocatable, void* data, size_t numBytes) const
	{
		// Check that the allocation is cpu mappable
		if (!mMemManager.isMemoryMappable(allocatable.getAllocation())) {
			throw std::logic_error("Trying to map on unmappable memory!!!");
		}
		// Copy data
		void* mappedMem = mMemManager.mapMemory(allocatable.getAllocation());
		std::memcpy(mappedMem, data, numBytes);
		mMemManager.unmapMemory(allocatable.getAllocation());
	}



	vk::Semaphore Context::createSemaphore() const
	{
		vk::SemaphoreCreateInfo createInfo;
		
		return getDevice().createSemaphore(createInfo);
	}

	vk::Fence Context::createFence(bool signaled) const
	{
		vk::FenceCreateInfo createInfo(
			signaled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlagBits{}
		);

		return getDevice().createFence(createInfo);
	}

	void Context::destroy(vk::Semaphore semaphore) const
	{
		getDevice().destroySemaphore(semaphore);
	}

	void Context::destroy(vk::Fence fence) const
	{
		getDevice().destroyFence(fence);
	}

	void Context::waitIdle() const
	{
		getDevice().waitIdle();
	}

	const CommandPool* Context::getCommandPool(const CommandPoolTypes type) const
	{
		assert(type != CommandPoolTypes::NUM);

		return mCommandPools.data() + static_cast<size_t>(type);
	}

	void Context::destroy(const vk::RenderPass renderPass) const
	{
		getDevice().destroyRenderPass(renderPass);
	}

	void Context::destroy(const vk::Framebuffer framebuffer) const
	{
		getDevice().destroyFramebuffer(framebuffer);
	}

	void Context::createShaderModule(const char* fileName, vk::ShaderModule* module) const
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

		(*module) = getDevice().createShaderModule(createInfo);
	}

	void Context::destroy(const vk::ShaderModule module) const
	{
		getDevice().destroyShaderModule(module);
	}


	void Context::destroy()
	{
		destroyCommandPools();

		mMemManager.destroy();
		mDevice.destroy();
		mInstance.destroy();
	}

	void Context::createCommandPools()
	{

		mCommandPools.reserve(static_cast<size_t>(CommandPoolTypes::NUM));

		mCommandPools.push_back(CommandPool(getGraphicsFamilyIdx(), {}, getDevice()));


		if (getPresentFamilyIdx() == getGraphicsFamilyIdx())
		{
			mCommandPools.push_back(mCommandPools.back());
		}
		else {
			mCommandPools.push_back(CommandPool(getPresentFamilyIdx(), {}, getDevice()));
		}

	}

	void Context::destroyCommandPools()
	{

		mCommandPools[static_cast<size_t>(CommandPoolTypes::eGraphic)].destroy();
		if (getPresentFamilyIdx() != getGraphicsFamilyIdx()) {
			mCommandPools[static_cast<size_t>(CommandPoolTypes::ePresent)].destroy();
		}

	}

	void Context::pickAndCreatePysicalDevice(const vk::SurfaceKHR* surf)
	{
		const vk::Instance instance = getInstance();
		std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
		if (devices.empty()) {
			throw std::runtime_error("No Vulkan devices avaliable!!");
		}

		std::vector<const char*> deviceExtensions;
		if (mPresentQueueRequested) {
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		// Test first with the devices that are dedicated GPUs
		bool deviceChoosen = false;
		for (vk::PhysicalDevice device : devices) {
			vk::PhysicalDeviceProperties prop = device.getProperties();
			if (prop.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
				continue;
			}

			if (isDeviceSuitable(device, deviceExtensions,
				surf, mAnisotropySamplerEnabled)) 
			{
				mPhysicalDevice = device;
				deviceChoosen = true;
			}

		}

		if (!deviceChoosen) {
			throw std::runtime_error("Cannot pick physical device!!!");
		}

		// if anisotropy, query max samples
		if (mAnisotropySamplerEnabled) {
			mMsaaSamples = getMaxUsableSampleCount();
		}
	}

	void Context::createLogicalDevice(const vk::SurfaceKHR* surf)
	{
		QueueIndices indices = findQueueFamiliesIndices(mPhysicalDevice, surf);
		std::set<uint32_t> familyIndices = indices.getUniqueIndices();
		// Preamptively store results if true
		mGraphicsFamilyIdx = indices.graphicsFamily.value();
		mComputeFamilyIdx = indices.computeFamily.value();
		mTransferFamilyIdx = indices.transferFamily.value();
		if (indices.presentFamily.has_value()) {
			mPresentFamilyIdx = indices.presentFamily.value();
		}

		std::vector<vk::DeviceQueueCreateInfo> queuesCreateInfos(familyIndices.size());

		float priority = 1.0f;
		int i = 0;
		for (uint32_t familyidx : familyIndices) {
			vk::DeviceQueueCreateInfo& info = queuesCreateInfos[i];
			info.queueFamilyIndex = familyidx;
			info.queueCount = 1;
			info.pQueuePriorities = &priority;
			++i;
		}

		vk::PhysicalDeviceFeatures features;
		features.samplerAnisotropy = mAnisotropySamplerEnabled;

		std::vector<const char*> deviceExtensions;
		if (mPresentQueueRequested) {
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		vk::DeviceCreateInfo createInfo;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queuesCreateInfos.size());
		createInfo.pQueueCreateInfos = queuesCreateInfos.data();
		createInfo.pEnabledFeatures = &features;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (DebugVk::validationLayersVkEnabled) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(DebugVk::validationLayersArray.size());
			createInfo.ppEnabledLayerNames = DebugVk::validationLayersArray.data();
		}

		mDevice = mPhysicalDevice.createDevice(createInfo);
	}

	void Context::createQueues()
	{
		mGraphicsQueue = mDevice.getQueue(mGraphicsFamilyIdx, 0);
		mComputeQueue = mDevice.getQueue(mComputeFamilyIdx, 0);
		mTransferQueue = mDevice.getQueue(mTransferFamilyIdx, 0);
		if (mPresentQueueRequested) {
			mPresentQueue = mDevice.getQueue(mPresentFamilyIdx, 0);
		}
	}

	bool Context::isDeviceSuitable(vk::PhysicalDevice physicalDevice,
		const std::vector<const char*>& deviceExtensions,
		const vk::SurfaceKHR* surfaceToRequestSwapChain,
		bool requestAnisotropySampler)
	{
		// Check extension support
		bool extensionSupport = false;
		{
			std::vector<vk::ExtensionProperties> prop = physicalDevice.enumerateDeviceExtensionProperties();
			std::set<std::string> extSet(deviceExtensions.begin(), deviceExtensions.end());
			for (const char* ext : deviceExtensions) {
				extSet.erase(ext);
			}
			if (extSet.empty()) {
				extensionSupport = true;
			}
		}
		// Check for family indices
		bool familySupport = false;
		{
			QueueIndices indices = findQueueFamiliesIndices(
				physicalDevice,
				surfaceToRequestSwapChain);
			familySupport = indices.isComplete();
		}

		// Check feature support
		bool featureSupport = true;
		{
			const vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();
			if (requestAnisotropySampler && !features.samplerAnisotropy)
				featureSupport = false;
		}

		// Test for swap chain support
		bool swapChainAvailable = surfaceToRequestSwapChain == nullptr;
		if (!swapChainAvailable) {
			uint32_t numFormats;
			uint32_t numPresentModes;

			vk::Result res = physicalDevice.getSurfaceFormatsKHR(*surfaceToRequestSwapChain, &numFormats, nullptr, vk::DispatchLoaderStatic());

			numFormats = (res == vk::Result::eSuccess) ? numFormats : 0;

			res = physicalDevice.getSurfacePresentModesKHR(*surfaceToRequestSwapChain, &numPresentModes, nullptr, vk::DispatchLoaderStatic());

			numPresentModes = (res == vk::Result::eSuccess) ? numPresentModes : 0;

			swapChainAvailable = numFormats > 0 && numPresentModes > 0;
		}

		return extensionSupport && familySupport && featureSupport && swapChainAvailable;
	}

	Context::QueueIndices Context::findQueueFamiliesIndices(
		vk::PhysicalDevice physicalDevice,
		const vk::SurfaceKHR* surf)
	{
		QueueIndices indices;
		indices.takePresentIntoAccount = surf != nullptr;

		std::vector<vk::QueueFamilyProperties> families =
			physicalDevice.getQueueFamilyProperties();
		for (uint32_t i = 0; i < static_cast<uint32_t>(families.size() && !indices.isComplete()); ++i) {
			vk::QueueFamilyProperties& family = families[i];

			if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphicsFamily = i;
			}

			if (family.queueFlags & vk::QueueFlagBits::eCompute) {
				indices.computeFamily = i;
			}

			if (family.queueFlags & vk::QueueFlagBits::eTransfer) {
				indices.transferFamily = i;
			}

			// Request also surface support if surface provided
			if (surf != nullptr) {
				vk::Bool32 res = physicalDevice.getSurfaceSupportKHR(i, *surf);
				if (res) {
					indices.presentFamily = i;
				}
			}
		}

		// Try to assign a unique transfer queue
		for (uint32_t i = 0; i < static_cast<uint32_t>(families.size()); ++i) {
			vk::QueueFamilyProperties& family = families[i];

			if (family.queueFlags == vk::QueueFlagBits::eTransfer) {
				indices.transferFamily = i;
			}
		}

		return indices;
	}

	vk::SampleCountFlagBits Context::getMaxUsableSampleCount() const
	{
		vk::PhysicalDeviceProperties properties = mPhysicalDevice.getProperties();

		vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
		if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
		if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
		if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
		if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
		if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
		if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

		return vk::SampleCountFlagBits::e1;
	}

}; // namespace vkg
}; // namespace gr