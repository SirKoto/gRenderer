#pragma once

#include "AppInstance.h"
#include "memory/MemoryManager.h"
#include "memory/BufferTransferer.h"
#include "command/ResetCommandPool.h"
#include "command/FreeCommandPool.h"

#include "present/SwapChain.h"
#include "resources/Image2D.h"
#include "resources/Buffer.h"
#include "resources/DescriptorManager.h"

#include "command/CommandFlusher.h"

#include <optional>
#include <set>
#include <glm/glm.hpp>


namespace gr
{
namespace vkg
{
	class RenderContext
	{
	public:

		RenderContext() = default;

		RenderContext(const RenderContext& o) = default;

		void createInstance(std::vector<const char*> extensions = {},
			bool loadGLFWextensions = true);

		void createDevice(
			bool enableAnisotropySampler = true,
			const vk::SurfaceKHR* surfaceToRequestSwapChain = nullptr
		);

		void flushData();


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

		Image2D createImage2DColorAttachment(const vk::Extent2D& extent,
			uint32_t mipLevels,
			vk::SampleCountFlagBits numSamples,
			vk::Format format);

		Image2D create2DDepthAttachment(
			const vk::Extent2D& extent,
			vk::SampleCountFlagBits numSamples);

		static vk::Format getDepthFormat() { return vk::Format::eD32Sfloat; }


		vk::Sampler createSampler(vk::SamplerAddressMode addressMode) const;

		Buffer createVertexBuffer(size_t sizeInBytes) const;
		Buffer createIndexBuffer(size_t sizeInBytes) const;
		Buffer createStagingBuffer(size_t sizeInBytes) const;
		Buffer createUniformBuffer(size_t sizeInBytes) const;
		Buffer createCpuVisibleBuffer(size_t sizeInBytes, vk::BufferUsageFlags usageFlags) const;


		void transferDataToGPU(const Allocatable& allocatable, const void* data, size_t numBytes) const;
		// Transfer sequentaly multiple pointers to the same allocatable resource
		void transferDataToGPU(const Allocatable& allocatable, uint32_t numDatas, const void** datas, size_t* numBytes) const;

		void mapAllocatable(const Allocatable& allocatable, void** ptr) const;
		void unmapAllocatable(const Allocatable& allocatable) const;
		void flushAllocations(const VmaAllocation* allocations, uint32_t num);

		vk::Semaphore createSemaphore() const;
		vk::Semaphore createTimelineSemaphore(uint64_t initialValue = 0) const;

		vk::Fence createFence(bool signaled) const;

		void createShaderModule(const char* fileName,
			vk::ShaderModule* outModule,
			std::vector<uint32_t>* outSpirV = nullptr) const;

		void waitIdle() const;

		struct FrameCommandPools
		{
			ResetCommandPool graphicsPool;
			ResetCommandPool presentPool;
			ResetCommandPool transferTransientPool;
		};

		void createCommandPools(FrameCommandPools* pools) const;
		void destroyCommandPools(FrameCommandPools* pools) const;

		FreeCommandPool* getGraphicsFreeCommandPool() { return &mGraphicsCommandPool; }
		FreeCommandPool* getTransferFreeCommandPool() { return &mTransferCommandPool; }


		vk::Queue getGraphicsQueue() const { return mGraphicsQueue; }
		vk::Queue getComputeQueue() const { return mComputeQueue; }
		vk::Queue getTransferQueue() const { return mTransferQueue; }
		vk::Queue getPresentQueue() const { return mPresentQueue; }

		uint32_t getGraphicsFamilyIdx() const { return mGraphicsFamilyIdx; }
		uint32_t getComputeFamilyIdx() const { return mComputeFamilyIdx; }
		uint32_t getTransferFamilyIdx() const { return mTransferFamilyIdx; }
		uint32_t getPresentFamilyIdx() const { return mPresentFamilyIdx; }

		BufferTransferer* getTransferer() {
			return &mGraphicsBufferTransferer;
		}

		bool isPresentQueueCreated() const { return mPresentQueueRequested; }

		size_t padUniformBuffer(size_t size) const;
		vk::SampleCountFlagBits getMsaaSampleCount() const { return mMsaaSamples; }
		CommandFlusher* getCommandFlusher() { return &mCommandFlusher; }

		DescriptorManager& getDescriptorManager() { return mDescriptorManager; }
		const DescriptorManager& getDescriptorManager() const { return mDescriptorManager; }

		void allocateDescriptorSet(uint32_t num,
			const vk::DescriptorSetLayout layout,
			vk::DescriptorSet* outLayouts) {
			return mDescriptorManager.allocateDescriptorSets(*this, num, layout, outLayouts);
		}

		void freeDescriptorSet(vk::DescriptorSet set, vk::DescriptorSetLayout layout) {
			getDescriptorManager().freeDescriptorSet(set, layout);
		}

		void safeDestroyBuffer(Buffer& buffer) const;
		void destroy(const Buffer& buffer) const;

		void safeDestroyImage(Image& image) const;
		void destroy(const Image& image) const;

		void destroy(const vk::RenderPass renderPass) const;

		void destroy(const vk::Framebuffer framebuffer) const;

		void destroy(const vk::ShaderModule module) const;

		void destroy(vk::Semaphore semaphore) const;

		void destroy(vk::Fence fence) const;

		void destroy(vk::Sampler sampler) const;

		void destroy(vk::Pipeline pip) const { mDevice.destroyPipeline(pip); }
		
		void destroy(vk::PipelineLayout pipLayout) const { mDevice.destroyPipelineLayout(pipLayout); }

		void destroy(vk::DescriptorSetLayout setLayout) const { mDevice.destroyDescriptorSetLayout(setLayout); }
		
		void destroy();


		// Basic predefined Vulkan Elements
		struct BasicTransformUBO {
			glm::mat4 M, V, P;
		};

		vk::DescriptorSetLayout getBasicTransformLayout() const { return mBasicDescriptorSetLayout; }
		vk::DescriptorSetLayout getEmptyLayout() const { return mEmptyDescriptorSetLayout; }
		vk::DescriptorSet getEmptyDescriptorSet() const { return mEmptyDescriptorSet; }

	protected:
		AppInstance mInstance;
		vk::PhysicalDevice mPhysicalDevice;
		vk::Device mDevice;
		MemoryManager mMemManager;
		CommandFlusher mCommandFlusher;

		BufferTransferer mGraphicsBufferTransferer;
		DescriptorManager mDescriptorManager;

		// Device members
		vk::Queue mGraphicsQueue;
		vk::Queue mComputeQueue;
		vk::Queue mTransferQueue;
		vk::Queue mPresentQueue;

		FreeCommandPool mGraphicsCommandPool;
		FreeCommandPool mTransferCommandPool;


		uint32_t mGraphicsFamilyIdx, mComputeFamilyIdx, mTransferFamilyIdx, mPresentFamilyIdx;
		
		bool mAnisotropySamplerEnabled, mPresentQueueRequested;
		vk::SampleCountFlagBits mMsaaSamples = vk::SampleCountFlagBits::e1;
		vk::PhysicalDeviceProperties mPhysicalProperties;

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

		void createImage2D(
			const vk::Extent2D& extent,
			uint32_t mipLevels,
			vk::SampleCountFlagBits numSamples,
			vk::Format format,
			vk::ImageUsageFlags usage,
			vk::Image* outImage,
			VmaAllocation* outAlloc) const;

		// Basic VkElements
		vk::DescriptorSetLayout mBasicDescriptorSetLayout;
		vk::DescriptorSetLayout mEmptyDescriptorSetLayout;
		vk::DescriptorSet mEmptyDescriptorSet;

		void createBasicVkElements();
		void destroyBasicVkElements();

	};

}; // namespace vkg

}; // namespace gr
