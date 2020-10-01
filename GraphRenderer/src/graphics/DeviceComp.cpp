#include "DeviceComp.h"
#include "DebugVk.h"
#include <set>

DeviceComp::DeviceComp(
	const AppInstance& instance, 
	bool enableAnisotropySampler,
	const vk::SurfaceKHR* surfaceToRequestSwapChain)
{
	mAnisotropySamplerEnabled = enableAnisotropySampler;
	mPresentQueueRequested = surfaceToRequestSwapChain != nullptr;
	pickAndCreatePysicalDevice(instance, surfaceToRequestSwapChain);
	createLogicalDevice(surfaceToRequestSwapChain);
	createQueues();
}

void DeviceComp::destroy()
{
	mDevice.destroy();
}

void DeviceComp::pickAndCreatePysicalDevice(
	const AppInstance& instance_,
	const vk::SurfaceKHR* surfaceToRequestSwapChain)
{
	const vk::Instance& instance = instance_;
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

		if (isDeviceSuitable(device, deviceExtensions, surfaceToRequestSwapChain, mAnisotropySamplerEnabled)) {
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

void DeviceComp::createLogicalDevice(const vk::SurfaceKHR* surf)
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

void DeviceComp::createQueues()
{	
	mGraphicsQueue = mDevice.getQueue(mGraphicsFamilyIdx, 0);
	mComputeQueue = mDevice.getQueue(mComputeFamilyIdx, 0);
	mTransferQueue = mDevice.getQueue(mTransferFamilyIdx, 0);
	if (mPresentQueueRequested) {
		mPresentQueue = mDevice.getQueue(mPresentFamilyIdx, 0);
	}
}


bool DeviceComp::isDeviceSuitable(
	const vk::PhysicalDevice& device,
	const std::vector<const char*>& deviceExtensions,
	const vk::SurfaceKHR* surfaceToRequestSwapChain,
	bool requestAnisotropySampler) const
{
	// Check extension support
	bool extensionSupport = false;
	{
		std::vector<vk::ExtensionProperties> prop = device.enumerateDeviceExtensionProperties();
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
		QueueIndices indices = findQueueFamiliesIndices(device, surfaceToRequestSwapChain);
		familySupport = indices.isComplete();
	}

	// Check feature support
	bool featureSupport = true;
	{
		const vk::PhysicalDeviceFeatures features = device.getFeatures();
		if (requestAnisotropySampler && !features.samplerAnisotropy)
			featureSupport = false;
	}

	// Test for swap chain support
	bool swapChainAvailable = surfaceToRequestSwapChain == nullptr;
	if (!swapChainAvailable) {
		uint32_t numFormats;
		uint32_t numPresentModes;

		device.getSurfaceFormatsKHR(*surfaceToRequestSwapChain, &numFormats, nullptr,vk::DispatchLoaderStatic());

		device.getSurfacePresentModesKHR(*surfaceToRequestSwapChain, &numPresentModes, nullptr, vk::DispatchLoaderStatic());
		swapChainAvailable = numFormats > 0 && numPresentModes > 0;
	}

	return extensionSupport && familySupport && featureSupport && swapChainAvailable;
}

DeviceComp::QueueIndices DeviceComp::findQueueFamiliesIndices(const vk::PhysicalDevice& device,
	const vk::SurfaceKHR* surf) const
{
	QueueIndices indices;
	indices.takePresentIntoAccount = surf != nullptr;

	std::vector<vk::QueueFamilyProperties> families =
		device.getQueueFamilyProperties();
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
			vk::Bool32 res = device.getSurfaceSupportKHR(i, *surf);
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

vk::SampleCountFlagBits DeviceComp::getMaxUsableSampleCount() const {
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