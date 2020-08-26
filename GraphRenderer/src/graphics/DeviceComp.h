#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>
#include <set>
#include "AppInstance.h"

class DeviceComp
{
public:

	DeviceComp() = delete;

	DeviceComp(
		const AppInstance& instance,
		bool requestPresentQueue,
		bool enableAnisotropySampler,
		const vk::SurfaceKHR* surfaceToRequestSwapChain = nullptr);

	void destroy();
	

	operator vk::PhysicalDevice() const { return mPhysicalDevice; }
	operator vk::Device() const { return mDevice; }

	vk::Queue getGraphicsQueue() const { return mGraphicsQueue; }
	vk::Queue getComputeQueue() const { return mComputeQueue; }
	vk::Queue getTransferQueue() const { return mTransferQueue; }


	vk::SampleCountFlags getMsaaSampleCount() const { return mMsaaSamples; }

	vk::SampleCountFlagBits getMaxUsableSampleCount() const;

protected:

	vk::PhysicalDevice mPhysicalDevice;
	vk::Device mDevice;

	vk::Queue mGraphicsQueue;
	vk::Queue mComputeQueue;
	vk::Queue mTransferQueue;


	bool mAnisotropySamplerEnabled, mPresentQueueRequested;
	vk::SampleCountFlags mMsaaSamples;

	// Struct used to determine the queueIndices inside this device
	typedef struct QueueIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> computeFamily;
		std::optional<uint32_t> transferFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value();
		}

		std::set<uint32_t> getUniqueIndices() const {
			return {graphicsFamily.value(), computeFamily.value(), transferFamily.value()};
		}
	} QueueIndices;

	void pickAndCreatePysicalDevice(const AppInstance& instance,
		const vk::SurfaceKHR* surfaceToRequestSwapChain);

	void createLogicalDevice();

	void createQueues();

	bool isDeviceSuitable(const vk::PhysicalDevice& device,
		const std::vector<const char*>& deviceExtensions,
		const vk::SurfaceKHR* surfaceToRequestSwapChain,
		bool requestAnisotropySampler) const;

	QueueIndices findQueueFamiliesIndices(const vk::PhysicalDevice& device) const;
};

