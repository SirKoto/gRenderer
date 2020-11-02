#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>
#include <set>
#include "AppInstance.h"

namespace gr
{
namespace vkg
{

	class DeviceComp
	{
	public:

		DeviceComp() = default;

		// If a surface is provided, a present queue will be created
		DeviceComp(
			const AppInstance& instance,
			bool enableAnisotropySampler,
			const vk::SurfaceKHR* surfaceToRequestSwapChain = nullptr);

		void destroy();
	

		explicit operator vk::PhysicalDevice() const { return mPhysicalDevice; }
		explicit operator vk::Device() const { return mDevice; }

		vk::PhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
		vk::Device getVkDevice() const { return mDevice; }

		vk::Queue getGraphicsQueue() const { return mGraphicsQueue; }
		vk::Queue getComputeQueue() const { return mComputeQueue; }
		vk::Queue getTransferQueue() const { return mTransferQueue; }
		vk::Queue getPresentQueue() const { return mPresentQueue; }

		uint32_t getGraphicsFamilyIdx() const { return mGraphicsFamilyIdx; }
		uint32_t getComputeFamilyIdx() const { return mComputeFamilyIdx; }
		uint32_t getTransferFamilyIdx() const { return mTransferFamilyIdx; }
		uint32_t getPresentFamilyIdx() const { return mPresentFamilyIdx; }

		bool isPresentQueueCreated() const { return mPresentQueueRequested; }

		vk::SampleCountFlags getMsaaSampleCount() const { return mMsaaSamples; }

		vk::SampleCountFlagBits getMaxUsableSampleCount() const;

	protected:

		vk::PhysicalDevice mPhysicalDevice;
		vk::Device mDevice;

		vk::Queue mGraphicsQueue;
		vk::Queue mComputeQueue;
		vk::Queue mTransferQueue;
		vk::Queue mPresentQueue;

		uint32_t mGraphicsFamilyIdx, mComputeFamilyIdx, mTransferFamilyIdx, mPresentFamilyIdx;



		bool mAnisotropySamplerEnabled, mPresentQueueRequested;
		vk::SampleCountFlags mMsaaSamples;

		// Struct used to determine the queueIndices inside this device
		typedef struct QueueIndices {
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> computeFamily;
			std::optional<uint32_t> transferFamily;
			std::optional<uint32_t> presentFamily;

			bool takePresentIntoAccount = false;

			bool isComplete() {
				return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value()
					&& (!takePresentIntoAccount || presentFamily.has_value());
			}

			std::set<uint32_t> getUniqueIndices() const {
				return (takePresentIntoAccount ? 
					std::set<uint32_t>{graphicsFamily.value(), computeFamily.value(), transferFamily.value(), presentFamily.value()} :
					std::set<uint32_t>{graphicsFamily.value(), computeFamily.value(), transferFamily.value()});
			}
		} QueueIndices;

		void pickAndCreatePysicalDevice(const AppInstance& instance,
			const vk::SurfaceKHR* surfaceToRequestSwapChain);

		void createLogicalDevice(const vk::SurfaceKHR* surf = nullptr);

		void createQueues();

		bool isDeviceSuitable(const vk::PhysicalDevice& device,
			const std::vector<const char*>& deviceExtensions,
			const vk::SurfaceKHR* surfaceToRequestSwapChain,
			bool requestAnisotropySampler) const;

		QueueIndices findQueueFamiliesIndices(const vk::PhysicalDevice& device,
			const vk::SurfaceKHR* surf) const;
	};


}; // namespace vkg
}; // namespace gr


