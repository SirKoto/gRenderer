#include "RenderContext.h"

#include "../utils/FsTools.h"
#include "DebugVk.h"

#include <glm/glm.hpp>

namespace gr
{
namespace vkg
{

void RenderContext::createInstance(std::vector<const char*> extensions, bool loadGLFWextensions)
{ 
	mInstance.create(extensions, loadGLFWextensions);
}

void RenderContext::createDevice(bool enableAnisotropySampler,
	const vk::SurfaceKHR* surfaceToRequestSwapChain)
{
	mAnisotropySamplerEnabled = enableAnisotropySampler;
	mPresentQueueRequested = surfaceToRequestSwapChain != nullptr;
	pickAndCreatePysicalDevice(surfaceToRequestSwapChain);
	createLogicalDevice(surfaceToRequestSwapChain);
	createQueues();

	mMemManager = MemoryManager(mInstance, mPhysicalDevice, mDevice);

	mGraphicsBufferTransferer.setUpTransferBlocks(this);
}

void RenderContext::flushData()
{
	mGraphicsCommandPool.flushFrees();
	mTransferCommandPool.flushFrees();
}


	Image2D RenderContext::createTexture2D(
		const vk::Extent2D& extent,
		uint32_t mipLevels,
		vk::SampleCountFlagBits numSamples,
		vk::Format format,
		vk::ImageAspectFlags imageAspect)
	{


		vk::ImageCreateInfo createInfo(
			{},							// flags
			vk::ImageType::e2D,			// Image Type
			format,						// Format
			vk::Extent3D(extent, 1),	// Extent
			mipLevels,					// Mip levels
			1,							// Array layers
			numSamples,					// SampleCount
			vk::ImageTiling::eOptimal,	// Image Tiling
			vk::ImageUsageFlagBits::eTransferDst |
			vk::ImageUsageFlagBits::eSampled,	// Image usage
			vk::SharingMode::eExclusive,
			0, nullptr, // concurrent families
			vk::ImageLayout::eUndefined	// initial layout
		);						

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

	void RenderContext::safeDestroyImage(Image& image)
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

	vk::Sampler RenderContext::createSampler(vk::SamplerAddressMode addressMode) const
	{
		vk::SamplerCreateInfo createInfo(
			vk::SamplerCreateFlags{},					// flags
			vk::Filter::eLinear, vk::Filter::eLinear,   // mag&min filter
			vk::SamplerMipmapMode::eNearest,			// mip map
			addressMode, addressMode, addressMode,		// address mode uvw
			0,											// mip bias
			true, 16									// anisotropy
			// ... compare ops
		);

		return mDevice.createSampler(createInfo);
	}

	Buffer RenderContext::createIndexBuffer(size_t sizeInBytes) const
	{
		vk::BufferCreateInfo createInfo(
			vk::BufferCreateFlagBits(),	// flags
			sizeInBytes,				// size of buffer
			vk::BufferUsageFlagBits::eIndexBuffer |
			vk::BufferUsageFlagBits::eTransferDst,
			vk::SharingMode::eExclusive, // To use with graphics and transfer queue
			0, nullptr
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

	Buffer RenderContext::createVertexBuffer(size_t sizeInBytes) const
	{
		vk::BufferCreateInfo createInfo(
			vk::BufferCreateFlagBits(),	// flags
			sizeInBytes,				// size of buffer
			vk::BufferUsageFlagBits::eVertexBuffer | 
				vk::BufferUsageFlagBits::eTransferDst,
			vk::SharingMode::eExclusive,
			0, nullptr
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

	Buffer RenderContext::createStagingBuffer(size_t sizeInBytes) const
	{
		vk::BufferCreateInfo createInfo(
			vk::BufferCreateFlagBits(),	// flags
			sizeInBytes,				// size of buffer
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::SharingMode::eExclusive, // To use with graphics and transfer queue
			0, nullptr					// family sharing
		);

		vk::Buffer buffer;
		VmaAllocation alloc;

		mMemManager.createBufferAllocation(createInfo,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			&buffer,
			&alloc);


		return Buffer(buffer, alloc);
	}

	Buffer RenderContext::createUniformBuffer(size_t sizeInBytes) const
	{
		std::array<const uint32_t, 2> sharedR = { getGraphicsFamilyIdx(), getTransferFamilyIdx() };
		vk::BufferCreateInfo createInfo(
			vk::BufferCreateFlagBits(),	// flags
			sizeInBytes,				// size of buffer
			vk::BufferUsageFlagBits::eUniformBuffer |
			vk::BufferUsageFlagBits::eTransferDst,
			vk::SharingMode::eConcurrent, // To use with graphics and transfer queue
			static_cast<uint32_t>(sharedR.size()),
			sharedR.data()
		);

		vk::Buffer buffer;
		VmaAllocation alloc;

		mMemManager.createBufferAllocation(createInfo,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			&buffer,
			&alloc);


		return Buffer(buffer, alloc);
	}


	void RenderContext::safeDestroyBuffer(Buffer& buffer) const
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

	void RenderContext::destroy(Buffer& buffer) const
	{
		mMemManager.freeAllocation(buffer.getAllocation());
		getDevice().destroyBuffer(buffer.getVkBuffer());
	}

	void RenderContext::transferDataToGPU(const Allocatable& allocatable, const void* data, size_t numBytes) const
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

	void RenderContext::transferDataToGPU(const Allocatable& allocatable, uint32_t numDatas, const void** datas, size_t* numBytes) const
	{
		// Check that the allocation is cpu mappable
		if (!mMemManager.isMemoryMappable(allocatable.getAllocation())) {
			throw std::logic_error("Trying to map on unmappable memory!!!");
		}
		void* mappedMem = mMemManager.mapMemory(allocatable.getAllocation());

		size_t offset = 0;
		for (uint32_t i = 0; i < numDatas; ++i) {
			std::memcpy( reinterpret_cast<char*>(mappedMem) + offset,
				datas[i],
				numBytes[i]);
			offset += numBytes[i];
		}

		mMemManager.unmapMemory(allocatable.getAllocation());

	}

	void RenderContext::mapAllocatable(const Allocatable& allocatable, void** ptr) const
	{
		// Check that the allocation is cpu mappable
		if (!mMemManager.isMemoryMappable(allocatable.getAllocation())) {
			throw std::logic_error("Trying to map on unmappable memory!!!");
		}
		*ptr = mMemManager.mapMemory(allocatable.getAllocation());
	}

	void RenderContext::unmapAllocatable(const Allocatable& allocatable) const
	{
		mMemManager.unmapMemory(allocatable.getAllocation());
	}



	vk::Semaphore RenderContext::createSemaphore() const
	{
		vk::SemaphoreCreateInfo createInfo;
		
		return getDevice().createSemaphore(createInfo);
	}

	vk::Semaphore RenderContext::createTimelineSemaphore(uint64_t initialValue) const
	{
		vk::SemaphoreTypeCreateInfo typeInfo(
			vk::SemaphoreType::eTimeline,	// Semaphore Type
			initialValue					// Initial value
		);

		vk::SemaphoreCreateInfo createInfo;
		createInfo.setPNext(&typeInfo);

		return getDevice().createSemaphore(createInfo);
	}

	vk::Fence RenderContext::createFence(bool signaled) const
	{
		vk::FenceCreateInfo createInfo(
			signaled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlagBits{}
		);

		return getDevice().createFence(createInfo);
	}

	void RenderContext::destroy(vk::Semaphore semaphore) const
	{
		getDevice().destroySemaphore(semaphore);
	}

	void RenderContext::destroy(vk::Fence fence) const
	{
		getDevice().destroyFence(fence);
	}

	void RenderContext::destroy(vk::Sampler sampler) const
	{
		mDevice.destroySampler(sampler);
	}

	void RenderContext::waitIdle() const
	{
		getDevice().waitIdle();
	}

	void RenderContext::destroy(const vk::RenderPass renderPass) const
	{
		getDevice().destroyRenderPass(renderPass);
	}

	void RenderContext::destroy(const vk::Framebuffer framebuffer) const
	{
		getDevice().destroyFramebuffer(framebuffer);
	}

	void RenderContext::createShaderModule(const char* fileName, vk::ShaderModule* module) const
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

	void RenderContext::destroy(const vk::ShaderModule module) const
	{
		getDevice().destroyShaderModule(module);
	}


	void RenderContext::destroy()
	{
		mGraphicsBufferTransferer.destroy(this);


		mGraphicsCommandPool.destroy();
		mTransferCommandPool.destroy();

		mMemManager.destroy();
		mDevice.destroy();
		mInstance.destroy();
	}

	RenderContext::FrameCommandPools RenderContext::createCommandPools() const
	{
		ResetCommandPool gp(getGraphicsFamilyIdx(), {}, getDevice());

		ResetCommandPool pp;

		if (getPresentFamilyIdx() == getGraphicsFamilyIdx())
		{
			pp = gp;
		}
		else {
			pp = ResetCommandPool(getPresentFamilyIdx(), {}, getDevice());
		}

		if (getGraphicsFamilyIdx() == getTransferFamilyIdx()) {
			throw std::runtime_error("Graphics family is the same as the transfer family!!");
		}

		ResetCommandPool tp(
			getTransferFamilyIdx(),
			vk::CommandPoolCreateFlagBits::eTransient,
			getDevice());

		FrameCommandPools pools;
		pools.graphicsPool = gp;
		pools.presentPool = pp;
		pools.transferTransientPool = tp;

		return pools;

	}

	void RenderContext::destroyCommandPools(FrameCommandPools* pools) const
	{
		std::set<vk::CommandPool> alreadyDestroyed;
		alreadyDestroyed.insert(pools->graphicsPool.get());
		pools->graphicsPool.destroy();

		if (alreadyDestroyed.count(pools->presentPool.get()) == 0) {
			pools->presentPool.destroy();
		}

		pools->transferTransientPool.destroy();
	}

	void RenderContext::pickAndCreatePysicalDevice(const vk::SurfaceKHR* surf)
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

	void RenderContext::createLogicalDevice(const vk::SurfaceKHR* surf)
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
		vk::PhysicalDeviceVulkan12Features features12;
		features12.timelineSemaphore = true;

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
		createInfo.setPNext(&features12);

		if (DebugVk::validationLayersVkEnabled) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(DebugVk::validationLayersArray.size());
			createInfo.ppEnabledLayerNames = DebugVk::validationLayersArray.data();
		}

		mDevice = mPhysicalDevice.createDevice(createInfo);
	}

	void RenderContext::createQueues()
	{
		mGraphicsQueue = mDevice.getQueue(mGraphicsFamilyIdx, 0);
		mComputeQueue = mDevice.getQueue(mComputeFamilyIdx, 0);
		mTransferQueue = mDevice.getQueue(mTransferFamilyIdx, 0);
		if (mPresentQueueRequested) {
			mPresentQueue = mDevice.getQueue(mPresentFamilyIdx, 0);
		}

		mCommandFlusher = CommandFlusher(mGraphicsQueue, mTransferQueue);

		mGraphicsCommandPool = FreeCommandPool(mGraphicsFamilyIdx, {}, getDevice());
		mTransferCommandPool = FreeCommandPool(mTransferFamilyIdx, {}, getDevice());

	}

	bool RenderContext::isDeviceSuitable(vk::PhysicalDevice physicalDevice,
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

	RenderContext::QueueIndices RenderContext::findQueueFamiliesIndices(
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

		// Try to assign unique compute queue
		for (uint32_t i = 0; i < static_cast<uint32_t>(families.size()); ++i) {
			vk::QueueFamilyProperties& family = families[i];

			if (family.queueFlags & vk::QueueFlagBits::eCompute &&
				i != indices.graphicsFamily.value() &&
				i != indices.presentFamily.value()) {
				indices.computeFamily = i;
				break;
			}
		}
		// Try to assign a unique transfer queue
		for (uint32_t i = 0; i < static_cast<uint32_t>(families.size()); ++i) {
			vk::QueueFamilyProperties& family = families[i];

			if (family.queueFlags & vk::QueueFlagBits::eTransfer &&
				i != indices.computeFamily.value() &&
				!(family.queueFlags & vk::QueueFlagBits::eGraphics)) {
				indices.transferFamily = i;
				break;
			}
		}

		return indices;
	}

	vk::SampleCountFlagBits RenderContext::getMaxUsableSampleCount() const
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