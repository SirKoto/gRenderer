#include "Engine.h"

#ifdef _WIN32 // TO avoid APIENTRY redefinition warning
#include <Windows.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "graphics/AppInstance.h"
#include "graphics/DeviceComp.h"
#include "graphics/memory/MemoryManager.h"
#include "graphics/render/RenderPassBuilder.h"

namespace gr
{

	void Engine::init()
	{
		glfwInit();
	}

	void Engine::terminate()
	{
		glfwTerminate();
	}

	void Engine::run()
	{
		using namespace vkg;

		AppInstance instance;
		mWindow.createVkSurface(instance);

		DeviceComp device(instance, true, &mWindow.getSurface());

		MemoryManager memManager(instance.getInstance(),
			device.getPhysicalDevice(),
			device.getVkDevice());

		mSwapChain = SwapChain(device, mWindow);

		mContext = Context(std::move(device), std::move(memManager));


		Image2D image = mContext.createDeviceImage2D({ 800,600 },
			1,
			vk::SampleCountFlagBits::e1,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eSampled);


		createAndRecordPresentCommandBuffers();
		createRenderPass();
		mPresentFramebuffers = mSwapChain.createFramebuffersOfSwapImages(mRenderPass);

		mImageAvailableSemaphore = mContext.createSemaphore();
		mRenderingFinishedSemaphore = mContext.createSemaphore();



		while (!mWindow.windowShouldClose()) {

			draw();
			Window::pollEvents();
		}

		// Destroy everything

		mContext.waitIdle();

		mContext.safeDestroyImage(image);

		mContext.destroySemaphore(mImageAvailableSemaphore);
		mContext.destroySemaphore(mRenderingFinishedSemaphore);

		deletePresentCommandBuffers();
		for (vk::Framebuffer frambuffer : mPresentFramebuffers) {
			mContext.destroyFramebuffer(frambuffer);
		}

		mContext.destroyRenderPass(mRenderPass);
		mSwapChain.destroy();
		mContext.destroy();
		mWindow.destroy(instance);
		instance.destroy();
	}

	void Engine::draw()
	{
		const vkg::CommandPool* cmdPool = mContext.getCommandPool(vkg::Context::CommandPoolTypes::ePresent);

		uint32_t imageIdx;
		bool outOfDateSwapChain = !mSwapChain.acquireNextImageBlock(mImageAvailableSemaphore, &imageIdx);

		if (outOfDateSwapChain) {
			recreateSwapChain();
			return;
		}

		cmdPool->submitCommandBuffer(
			mPresentCommandBuffers[imageIdx],
			&mImageAvailableSemaphore,
			vk::PipelineStageFlagBits::eTransfer,
			&mRenderingFinishedSemaphore);

		bool swapChainNeedsRecreation = !cmdPool->submitPresentationImage(
			mSwapChain.getVkSwapChain(),
			imageIdx,
			&mRenderingFinishedSemaphore
		);

		if (swapChainNeedsRecreation) {
			recreateSwapChain();
		}
	}

	void Engine::createRenderPass()
	{
		vkg::RenderPassBuilder builder;
		builder.reserveNumAttachmentDescriptions(1);
		builder.reserveNumDependencies(0);
		builder.reserveNumSubpassDescriptions(1);

		vk::AttachmentReference presentRef = builder.pushColorAttachmentDescription(
			mSwapChain.getFormat(),			// Format
			vk::SampleCountFlagBits::e1,	// Sample Count per pixel
			vk::AttachmentLoadOp::eClear,	// Load operation 
			vk::AttachmentStoreOp::eStore,	// Store operaton
			vk::ImageLayout::ePresentSrcKHR,// Initial layout
			vk::ImageLayout::ePresentSrcKHR // Final layout
		);

		uint32_t presentSubPass = builder.pushGraphicsSubpassDescriptionSimple(
			&presentRef
		);

		mRenderPass =  builder.buildRenderPass(mContext.getDeviceComp());
	}

	void Engine::createAndRecordPresentCommandBuffers()
	{

		const vkg::CommandPool* cmdPool = mContext.getCommandPool(vkg::Context::CommandPoolTypes::ePresent);

		mPresentCommandBuffers = cmdPool->createCommandBuffers(mSwapChain.getNumImages());


		const uint32_t presentQueueFamilyIdx = mContext.getQueueFamilyIndex(vkg::Context::CommandPoolTypes::ePresent);

		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

		vk::ClearColorValue clearColor;
		clearColor.setFloat32({ 1.0f, 0.8f, 0.4f, 0.0f });

		// range of the image that will be modified
		vk::ImageSubresourceRange imageRange(
			vk::ImageAspectFlagBits::eColor,	// aspect mask
			0, 1,	// base mip level and count
			0, 1	// base Array layer and count
		);

		vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier(
			{}, {}, {}, {}, // src/dest masks and layouts	
			presentQueueFamilyIdx,	// src Family
			presentQueueFamilyIdx,	// dst Family
			nullptr,				// image
			imageRange);			// range


		const uint32_t num = static_cast<uint32_t>(mPresentCommandBuffers.size());
		assert(num == mSwapChain.getNumImages());

		for (uint32_t i = 0; i < num; ++i) {

			// Begin command buffer recording
			vk::CommandBuffer commandBuffer = mPresentCommandBuffers[i];
			commandBuffer.begin(beginInfo);

			// Set actual image to barrier
			barrier.setImage(mSwapChain.getImagesVector()[i]);

			// from present to clear barrier
			barrier
				.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
				.setDstAccessMask(vk::AccessFlagBits::eMemoryWrite)
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal);

			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,	// src stage
				vk::PipelineStageFlagBits::eTransfer,	// dst stage
				{},										// dependency flags
				0, nullptr,								// memory barrier
				0, nullptr,								// buffer barrier
				1, &barrier								// image barrier
			);

			// Clear image with clear color
			commandBuffer.clearColorImage(
				barrier.image,							// image
				vk::ImageLayout::eTransferDstOptimal,	// image actual layout
				&clearColor,							// color to use
				1, &imageRange							// image subresource range
			);

			// from clear to present barrier
			barrier
				.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite)
				.setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(vk::ImageLayout::ePresentSrcKHR);

			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,	// src stage
				vk::PipelineStageFlagBits::eTransfer,	// dst stage
				{},										// dependency flags
				0, nullptr,								// memory barrier
				0, nullptr,								// buffer barrier
				1, &barrier								// image barrier
			);

			commandBuffer.end();
		}
	}

	void Engine::deletePresentCommandBuffers()
	{
		const vkg::CommandPool* cmdPool = mContext.getCommandPool(vkg::Context::CommandPoolTypes::ePresent);
		cmdPool->free(mPresentCommandBuffers.data(), static_cast<uint32_t>(mPresentCommandBuffers.size()));
	}

	void Engine::recreateSwapChain()
	{
		while (mWindow.isWindowMinimized()) {
			mWindow.waitEvents();
		}

		mContext.waitIdle();
		deletePresentCommandBuffers();

		mSwapChain.recreateSwapChain(mContext.getDeviceComp(), mWindow);

		createAndRecordPresentCommandBuffers();

		for (vk::Framebuffer frambuffer : mPresentFramebuffers) {
			mContext.destroyFramebuffer(frambuffer);
		}

		mPresentFramebuffers = mSwapChain.createFramebuffersOfSwapImages(mRenderPass);
	}

}; // namespace gr