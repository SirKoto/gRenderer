#pragma once

#include "AppInstance.h"
#include "memory/MemoryManager.h"
#include "command/ResetCommandPool.h"
#include "present/SwapChain.h"
#include "resources/Image2D.h"
#include "resources/Buffer.h"
#include "command/CommandFlusher.h"

#include <optional>
#include <set>


namespace gr
{
namespace vkg
{
	class RenderContext
	{
	public:

		RenderContext(std::vector<const char*> extensions = {},
			bool loadGLFWextensions = true);

		RenderContext(const RenderContext& o) = default;

		void createDevice(
			bool enableAnisotropySampler = true,
			const vk::SurfaceKHR* surfaceToRequestSwapChain = nullptr
		);


		const vk::PhysicalDevice &getPhysicalDevice() const { return mPhysicalDevice; }
		const vk::Device &getDevice() const { return mDevice; }
		const vk::Instance &getInstance() const { return mInstance.getInstance(); }
		explicit operator vk::Instance() const { return mInstance.getInstance(); }
		explicit operator vk::PhysicalDevice() const { return mPhysicalDevice; }
		explicit operator vk::Device() const { return mDevice; }

		Image2D createTexture2D(const vk::Extent2D& extent,
			uint32_t mipLevels,
			vk::SampleCountFlagBits numSamples,
			vk::Format format,
			vk::ImageAspectFlags ImageAspect = vk::ImageAspectFlagBits::eColor);

		void safeDestroyImage(Image& image);

		vk::Sampler createSampler(vk::SamplerAddressMode addressMode) const;

		Buffer createVertexBuffer(size_t sizeInBytes) const;
		Buffer createIndexBuffer(size_t sizeInBytes) const;
		Buffer createStagingBuffer(size_t sizeInBytes) const;
		Buffer createUniformBuffer(size_t sizeInBytes) const;

		void safeDestroyBuffer(Buffer& buffer);

		void transferDataToGPU(const Allocatable& allocatable, const void* data, size_t numBytes) const;
		// Transfer sequentaly multiple pointers to the same allocatable resource
		void transferDataToGPU(const Allocatable& allocatable, uint32_t numDatas, const void** datas, size_t* numBytes) const;


		vk::Semaphore createSemaphore() const;
		vk::Semaphore createTimelineSemaphore(uint64_t initialValue = 0) const;

		vk::Fence createFence(bool signaled) const;

		void createShaderModule(const char* fileName, vk::ShaderModule* module) const;

		void waitIdle() const;

		struct CommandPools
		{
			ResetCommandPool graphicsPool;
			ResetCommandPool presentPool;
			ResetCommandPool transferTransientPool;
		};

		CommandPools createCommandPools() const;
		void destroyCommandPools(CommandPools* pools) const;


		vk::Queue getGraphicsQueue() const { return mGraphicsQueue; }
		vk::Queue getComputeQueue() const { return mComputeQueue; }
		vk::Queue getTransferQueue() const { return mTransferQueue; }
		vk::Queue getPresentQueue() const { return mPresentQueue; }

		uint32_t getGraphicsFamilyIdx() const { return mGraphicsFamilyIdx; }
		uint32_t getComputeFamilyIdx() const { return mComputeFamilyIdx; }
		uint32_t getTransferFamilyIdx() const { return mTransferFamilyIdx; }
		uint32_t getPresentFamilyIdx() const { return mPresentFamilyIdx; }

		bool isPresentQueueCreated() const { return mPresentQueueRequested; }

		vk::SampleCountFlagBits getMsaaSampleCount() const { return mMsaaSamples; }
		CommandFlusher* getCommandFlusher() { return &mCommandFlusher; }

		void destroy(const vk::RenderPass renderPass) const;

		void destroy(const vk::Framebuffer framebuffer) const;

		void destroy(const vk::ShaderModule module) const;

		void destroy(vk::Semaphore semaphore) const;

		void destroy(vk::Fence fence) const;

		void destroy(vk::Sampler sampler) const;

		void destroy();

	protected:
		AppInstance mInstance;
		vk::PhysicalDevice mPhysicalDevice;
		vk::Device mDevice;
		MemoryManager mMemManager;
		CommandFlusher mCommandFlusher;

		// Device members
		vk::Queue mGraphicsQueue;
		vk::Queue mComputeQueue;
		vk::Queue mTransferQueue;
		vk::Queue mPresentQueue;


		uint32_t mGraphicsFamilyIdx, mComputeFamilyIdx, mTransferFamilyIdx, mPresentFamilyIdx;
		
		bool mAnisotropySamplerEnabled, mPresentQueueRequested;
		vk::SampleCountFlagBits mMsaaSamples = vk::SampleCountFlagBits::e1;

		// Device creation
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
					std::set<uint32_t>{ graphicsFamily.value(), computeFamily.value(), transferFamily.value() });
			}
		} QueueIndices;

		void pickAndCreatePysicalDevice(const vk::SurfaceKHR* surfaceToRequestSwapChain);

		void createLogicalDevice(const vk::SurfaceKHR* surf = nullptr);

		void createQueues();

		static bool isDeviceSuitable(
			vk::PhysicalDevice physicalDevice,
			const std::vector<const char*>& deviceExtensions,
			const vk::SurfaceKHR* surfaceToRequestSwapChain,
			bool requestAnisotropySampler);

		static QueueIndices findQueueFamiliesIndices(
			vk::PhysicalDevice physicalDevice,
			const vk::SurfaceKHR* surf);

		vk::SampleCountFlagBits getMaxUsableSampleCount() const;


	};

}; // namespace vkg

}; // namespace gr
