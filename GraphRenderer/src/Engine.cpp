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
#include "graphics/render/GraphicsPipelineBuilder.h"

#include "utils/grjob.h"

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

		{
			DeviceComp device(instance, true, &mWindow.getSurface());

			MemoryManager memManager(instance.getInstance(),
				device.getPhysicalDevice(),
				device.getVkDevice());


			mContext = Context(std::move(device), std::move(memManager));
		}

		mSwapChain = SwapChain(mContext.getDeviceComp(), mWindow);

		createSyncObjects();

		Image2D image = mContext.createDeviceImage2D({ 800,600 },
			1,
			vk::SampleCountFlagBits::e1,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eSampled);

		
		createRenderPass();
		mPresentFramebuffers = mSwapChain.createFramebuffersOfSwapImages(mRenderPass);

		createShaderModules();
		createPipelineLayout();
		createGraphicsPipeline();

		createAndRecordGraphicCommandBuffers();
		//createAndRecordPresentCommandBuffers();

		while (!mWindow.windowShouldClose()) {

			draw();
			Window::pollEvents();
		}

		// Destroy everything

		mContext.waitIdle();

		mContext.safeDestroyImage(image);

		//deletePresentCommandBuffers();

		cleanup();

		mSwapChain.destroy();
		mContext.destroy();
		mWindow.destroy(instance);
		instance.destroy();
	}

	void Engine::draw()
	{
		vk::Result res;
		// advance frame
		mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		// Wait for current frame, if it was still being executed
		res = mContext.getVkDevice().waitForFences(1, mInFlightFences.data() + mCurrentFrame, true, UINT32_MAX);
		assert(res == vk::Result::eSuccess);

		const vkg::CommandPool* cmdPool = mContext.getCommandPool(vkg::Context::CommandPoolTypes::eGraphic);

		uint32_t imageIdx;
		bool outOfDateSwapChain = !mSwapChain.acquireNextImageBlock(
			mImageAvailableSemaphores[mCurrentFrame], &imageIdx);

		if (outOfDateSwapChain) {
			recreateSwapChain();
			return;
		}

		// maybe out of order next image, thus wait also for next image
		if (mImagesInFlightFences[imageIdx]) {
			res = mContext.getVkDevice().waitForFences(1, mImagesInFlightFences.data() + imageIdx, true, UINT32_MAX);
			assert(res == vk::Result::eSuccess);
		}
		// update in flight image
		mImagesInFlightFences[imageIdx] = mInFlightFences[mCurrentFrame];

		mContext.getVkDevice().resetFences(1, mInFlightFences.data() + mCurrentFrame);

		cmdPool->submitCommandBuffer(
			mGraphicCommandBuffers[imageIdx],
			&mImageAvailableSemaphores[mCurrentFrame],
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			&mRenderingFinishedSemaphores[mCurrentFrame],
			mInFlightFences[mCurrentFrame]);

		bool swapChainNeedsRecreation = 
			!mContext.getCommandPool(vkg::Context::CommandPoolTypes::ePresent)->submitPresentationImage(
				mSwapChain.getVkSwapChain(),
				imageIdx,
				&mRenderingFinishedSemaphores[mCurrentFrame]
			);

		if (swapChainNeedsRecreation) {
			recreateSwapChain();
		}
	}

	void Engine::createRenderPass()
	{
		vkg::RenderPassBuilder builder;
		builder.reserveNumAttachmentDescriptions(1);
		builder.reserveNumDependencies(1);
		builder.reserveNumSubpassDescriptions(1);

		vk::AttachmentReference presentRef = builder.pushColorAttachmentDescription(
			mSwapChain.getFormat(),			// Format
			vk::SampleCountFlagBits::e1,	// Sample Count per pixel
			vk::AttachmentLoadOp::eClear,	// Load operation 
			vk::AttachmentStoreOp::eStore,	// Store operaton
			vk::ImageLayout::eUndefined,	// Initial layout
			vk::ImageLayout::ePresentSrcKHR // Final layout
		);

		uint32_t presentSubPass = builder.pushGraphicsSubpassDescriptionSimple(
			&presentRef
		);

		assert(0 == presentSubPass);

		vk::SubpassDependency dependency(
			VK_SUBPASS_EXTERNAL,	// src Stage
			0,			// dst Stage
			vk::PipelineStageFlagBits::eColorAttachmentOutput, // src Stage Mask
			vk::PipelineStageFlagBits::eColorAttachmentOutput, // dst Stage Mask
			{}, vk::AccessFlagBits::eColorAttachmentWrite
		);

		builder.pushSubpassDependency(dependency);

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

		cleanupSwapChainDependantObjs();

		mSwapChain.recreateSwapChain(mContext.getDeviceComp(), mWindow);

		createRenderPass();

		mPresentFramebuffers = mSwapChain.createFramebuffersOfSwapImages(mRenderPass);

		createGraphicsPipeline();
		createAndRecordGraphicCommandBuffers();
	}

	void Engine::createAndRecordGraphicCommandBuffers()
	{
		const vkg::CommandPool* cmdPool = mContext.getCommandPool(vkg::Context::CommandPoolTypes::eGraphic);

		mGraphicCommandBuffers = cmdPool->createCommandBuffers(mSwapChain.getNumImages());


		const uint32_t graphicsQueueFamilyIdx = mContext.getQueueFamilyIndex(vkg::Context::CommandPoolTypes::eGraphic);

		vk::CommandBufferBeginInfo beginInfo = {};

		for (uint32_t i = 0; i < mSwapChain.getNumImages(); ++i) {
			vk::CommandBuffer buff = mGraphicCommandBuffers[i];
			buff.begin(beginInfo);

			std::array<vk::ClearValue, 1> clearVal = {};
			clearVal[0].color.setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });

			vk::RenderPassBeginInfo passInfo(
				mRenderPass,
				mPresentFramebuffers[i],
				vk::Rect2D(vk::Offset2D{0, 0},
					mSwapChain.getExtent()),
				clearVal
			);

			buff.beginRenderPass(passInfo, vk::SubpassContents::eInline);

			buff.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline);

			buff.draw(3, 1, 0, 0); // num vert, instance, first vertex, first instance

			buff.endRenderPass();

			buff.end();
		}

	}

	void Engine::createShaderModules()
	{
		{
			grjob::Job jobs[2];
			jobs[0] = grjob::Job(&vkg::Context::createShaderModule, &mContext, "resources/shaders/SPIR-V/hardCodedTri.vert.spv", mShaderModules + 0);
			jobs[1] = grjob::Job(&vkg::Context::createShaderModule, &mContext, "resources/shaders/SPIR-V/test.frag.spv", mShaderModules + 1);
			grjob::Counter* c = nullptr;

			grjob::runJobBatch(gr::grjob::Priority::eMid, jobs, 2, &c);

			grjob::waitForCounterAndFree(c, 0);
		}
	}

	void Engine::createPipelineLayout()
	{
		vk::PipelineLayoutCreateInfo createInfo(
			{}, // flags
			0, nullptr, // set layouts
			0, nullptr // push constants
		);

		mPipLayout = mContext.getVkDevice().createPipelineLayout(createInfo);
	}

	void Engine::createGraphicsPipeline()
	{
		vkg::GraphicsPipelineBuilder builder;

		builder.setShaderStages(mShaderModules[0], mShaderModules[1]);
		// NO VertexBindingDescription
		builder.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
		builder.setViewportSize(mSwapChain.getExtent());
		builder.setMultisampleCount(vk::SampleCountFlagBits::e1);
		builder.setColorBlendAttachmentStd();
		builder.setPipelineLayout(mPipLayout);

		mGraphicsPipeline = builder.createPipeline(mContext.getVkDevice(), mRenderPass, 0);
	}

	void Engine::createSyncObjects()
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			mImageAvailableSemaphores[i] = mContext.createSemaphore();
			mRenderingFinishedSemaphores[i] = mContext.createSemaphore();
			mInFlightFences[i] = mContext.createFence(true);
		}

		mImagesInFlightFences.resize(mSwapChain.getNumImages());
	}

	void Engine::cleanup()
	{
		// destroy sync objects
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			mContext.destroy(mImageAvailableSemaphores[i]);
			mContext.destroy(mRenderingFinishedSemaphores[i]);
			mContext.destroy(mInFlightFences[i]);
		}

		cleanupSwapChainDependantObjs();
		mContext.getVkDevice().destroyPipelineLayout(mPipLayout);

		mContext.destroy(mShaderModules[0]);
		mContext.destroy(mShaderModules[1]);
	}

	void Engine::cleanupSwapChainDependantObjs()
	{
		for (vk::Framebuffer frambuffer : mPresentFramebuffers) {
			mContext.destroy(frambuffer);
		}

		deletePresentCommandBuffers();

		mContext.getVkDevice().destroyPipeline(mGraphicsPipeline);
		mContext.destroy(mRenderPass);
	}

}; // namespace gr