#include "Engine.h"

#ifdef _WIN32 // TO avoid APIENTRY redefinition warning
#include <Windows.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "graphics/AppInstance.h"
#include "graphics/DeviceComp.h"
#include "graphics/memory/MemoryManager.h"


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

		mAdmin = Admin(std::move(device), std::move(memManager));


		Image2D image = mAdmin.createDeviceImage2D({ 800,600 },
			1,
			vk::SampleCountFlagBits::e1,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eSampled);


		createAndRecordPresentCommandBuffers();

		vk::Semaphore imageAvailableSemaphore = mAdmin.createSemaphore();
		vk::Semaphore renderingFinishedSemaphore = mAdmin.createSemaphore();

		const vkg::CommandPool* cmdPool = mAdmin.getCommandPool(vkg::Admin::CommandPoolTypes::ePresent);


		while (!mWindow.windowShouldClose()) {

			uint32_t imageIdx;
			bool outOfDateSwapChain = !mSwapChain.acquireNextImageBlock(imageAvailableSemaphore, &imageIdx);

			if (outOfDateSwapChain) {
				recreateSwapChain();
				continue;
			}

			cmdPool->submitCommandBuffer(
				mPresentCommandBuffers[imageIdx],
				&imageAvailableSemaphore,
				vk::PipelineStageFlagBits::eTransfer,
				&renderingFinishedSemaphore);

			bool swapChainNeedsRecreation = !cmdPool->submitPresentationImage(
				mSwapChain.getVkSwapChain(),
				imageIdx,
				&renderingFinishedSemaphore
			);

			if (swapChainNeedsRecreation) {
				recreateSwapChain();
			}
			Window::pollEvents();
		}

		// Destroy everything

		mAdmin.waitIdle();

		mAdmin.safeDestroyImage(image);

		mAdmin.destroySemaphore(imageAvailableSemaphore);
		mAdmin.destroySemaphore(renderingFinishedSemaphore);

		deletePresentCommandBuffers();
		mSwapChain.destroy();
		mAdmin.destroy();
		mWindow.destroy(instance);
		instance.destroy();
	}

	void Engine::createAndRecordPresentCommandBuffers()
	{

		const vkg::CommandPool* cmdPool = mAdmin.getCommandPool(vkg::Admin::CommandPoolTypes::ePresent);

		mPresentCommandBuffers = cmdPool->createCommandBuffers(mSwapChain.getNumImages());


		const uint32_t presentQueueFamilyIdx = mAdmin.getQueueFamilyIndex(vkg::Admin::CommandPoolTypes::ePresent);

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
		const vkg::CommandPool* cmdPool = mAdmin.getCommandPool(vkg::Admin::CommandPoolTypes::ePresent);
		cmdPool->free(mPresentCommandBuffers.data(), static_cast<uint32_t>(mPresentCommandBuffers.size()));
	}

	void Engine::recreateSwapChain()
	{
		while (mWindow.isWindowMinimized()) {
			mWindow.waitEvents();
		}

		mAdmin.waitIdle();
		deletePresentCommandBuffers();

		mSwapChain.recreateSwapChain(*mAdmin.getDeviceComp(), mWindow);

		createAndRecordPresentCommandBuffers();
	}

}; // namespace gr