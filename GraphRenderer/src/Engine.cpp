#include "Engine.h"

#define GLM_FORCE_RADIANS

#ifdef _WIN32 // TO avoid APIENTRY redefinition warning
#include <Windows.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "graphics/AppInstance.h"
#include "graphics/memory/MemoryManager.h"
#include "graphics/render/RenderPassBuilder.h"
#include "graphics/render/GraphicsPipelineBuilder.h"
#include "graphics/shaders/VertexInputDescription.h"

#include "utils/grjob.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image/stb_image.h>
#include <chrono>


namespace gr
{

	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;
		glm::vec2 texCoord;
	};

	const std::vector<Vertex> vertices = {
	{ {-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
	{ {0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{ {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{ {-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};
	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};

	struct UBO {
		glm::mat4 M, V, P;
	};


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

		mWindow.createVkSurface(mContext.getInstance());

		mContext.createDevice(true, &mWindow.getSurface());

		mSwapChain = SwapChain(mContext, mWindow);

		createSyncObjects();

		
		createRenderPass();
		mPresentFramebuffers = mSwapChain.createFramebuffersOfSwapImages(mRenderPass);

		mDescriptorManager.initialize(mContext);

		createShaderModules();
		createBuffers();
		createUniformBuffers();
		createTextureImage();
		createDescriptorSetLayout();
		createDescriptorSets();
		createPipelineLayout();
		createGraphicsPipeline();


		createAndRecordGraphicCommandBuffers();

		while (!mWindow.windowShouldClose()) {

			draw();
			Window::pollEvents();
		}

		// Destroy everything

		mContext.waitIdle();


		cleanup();

		mSwapChain.destroy();
		mWindow.destroy(mContext.getInstance());
		mContext.destroy();
	}

	void Engine::draw()
	{
		vk::Result res;
		// advance frame
		mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		// Wait for current frame, if it was still being executed
		res = mContext.getDevice().waitForFences(1, mInFlightFences.data() + mCurrentFrame, true, UINT64_MAX);
		assert(res == vk::Result::eSuccess);

		const vkg::CommandPool& cmdPool = mContext.getGraphicsCommandPool();

		uint32_t imageIdx;
		bool outOfDateSwapChain = !mSwapChain.acquireNextImageBlock(
			mImageAvailableSemaphores[mCurrentFrame], &imageIdx);

		if (outOfDateSwapChain) {
			recreateSwapChain();
			return;
		}

		// maybe out of order next image, thus wait also for next image
		if (mImagesInFlightFences[imageIdx]) {
			res = mContext.getDevice().waitForFences(1, mImagesInFlightFences.data() + imageIdx, true, UINT64_MAX);
			assert(res == vk::Result::eSuccess);
		}
		// update in flight image
		mImagesInFlightFences[imageIdx] = mInFlightFences[mCurrentFrame];

		mContext.getDevice().resetFences(1, mInFlightFences.data() + mCurrentFrame);

		updateUBO(imageIdx);

		cmdPool.submitCommandBuffer(
			mGraphicCommandBuffers[imageIdx],
			&mImageAvailableSemaphores[mCurrentFrame],
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			&mRenderingFinishedSemaphores[mCurrentFrame],
			mInFlightFences[mCurrentFrame]);

		bool swapChainNeedsRecreation = 
			!mContext.getPresentCommandPool().submitPresentationImage(
				mSwapChain.getVkSwapChain(),
				imageIdx,
				&mRenderingFinishedSemaphores[mCurrentFrame]
			);

		if (swapChainNeedsRecreation) {
			recreateSwapChain();
		}
	}

	void Engine::updateUBO(uint32_t currentImage)
	{
		static auto start = std::chrono::high_resolution_clock::now();
		auto now = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(now - start).count();
		UBO ubo;
		ubo.M = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
			glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.V = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f,
			0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.P = glm::perspective(glm::radians(45.0f),
			mSwapChain.getExtent().width / static_cast<float>(mSwapChain.getExtent().height),
			0.1f, 10.0f);
		ubo.P[1][1] *= -1.0;

		mContext.transferDataToGPU(mUbos[currentImage], &ubo, sizeof(ubo));
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

		mRenderPass =  builder.buildRenderPass(mContext);
	}



	void Engine::recreateSwapChain()
	{
		while (mWindow.isWindowMinimized()) {
			mWindow.waitEvents();
		}

		mContext.waitIdle();

		cleanupSwapChainDependantObjs();

		mSwapChain.recreateSwapChain(mContext, mWindow);

		createRenderPass();

		mPresentFramebuffers = mSwapChain.createFramebuffersOfSwapImages(mRenderPass);

		createUniformBuffers();
		createDescriptorSets();
		createGraphicsPipeline();
		createAndRecordGraphicCommandBuffers();
	}

	void Engine::createAndRecordGraphicCommandBuffers()
	{
		const vkg::CommandPool& cmdPool = mContext.getGraphicsCommandPool();

		mGraphicCommandBuffers = cmdPool.createCommandBuffers(mSwapChain.getNumImages());


		const uint32_t graphicsQueueFamilyIdx = mContext.getGraphicsFamilyIdx();

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

			vk::DeviceSize offset = 0;
			buff.bindVertexBuffers(0, 1, &mVertexBuffer.getVkBuffer(), &offset);
			buff.bindIndexBuffer(mIndexBuffer.getVkBuffer(), 0, vk::IndexType::eUint16);

			buff.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				mPipLayout,
				0, 1, &mDescriptorSets[i],	// first set, descriptors sets
				0, nullptr					// dynamic offsets
			);

			buff.drawIndexed(
				static_cast<uint32_t>(indices.size()), // index count
				1, 0, 0, 0	// instance count, and offsets
			);

			buff.endRenderPass();

			buff.end();
		}

	}

	void Engine::createShaderModules()
	{
		{
			grjob::Job jobs[2];
			jobs[0] = grjob::Job(&vkg::Context::createShaderModule, &mContext, "resources/shaders/SPIR-V/sampler.vert.spv", mShaderModules + 0);
			jobs[1] = grjob::Job(&vkg::Context::createShaderModule, &mContext, "resources/shaders/SPIR-V/sampler.frag.spv", mShaderModules + 1);
			grjob::Counter* c = nullptr;

			grjob::runJobBatch(gr::grjob::Priority::eMid, jobs, 2, &c);

			grjob::waitForCounterAndFree(c, 0);
		}
	}

	void Engine::createDescriptorSetLayout()
	{
		std::array< vk::DescriptorSetLayoutBinding, 2> bindings;
		bindings[0] = vk::DescriptorSetLayoutBinding(
			0u, // binding
			vk::DescriptorType::eUniformBuffer,
			1,		// number of elements in the ubo (array)
			vk::ShaderStageFlagBits::eVertex,
			nullptr
		);
		bindings[1] = vk::DescriptorSetLayoutBinding(
			1u, // binding
			vk::DescriptorType::eCombinedImageSampler,
			1,		// descriptor count
			vk::ShaderStageFlagBits::eFragment,
			nullptr
		);

		vk::DescriptorSetLayoutCreateInfo createInfo(
			{}, // flags
			bindings
		);

		mDescriptorSetLayout = mContext.getDevice().createDescriptorSetLayout(createInfo);

	}

	void Engine::createDescriptorSets()
	{
		mDescriptorSets.resize(mSwapChain.getNumImages());

		mDescriptorManager.allocateDescriptorSets(
			mContext,
			mSwapChain.getNumImages(),
			mDescriptorSetLayout,
			mDescriptorSets.data()
		);

		for (uint32_t i = 0; i < mSwapChain.getNumImages(); ++i) {
			vk::DescriptorBufferInfo buffInfo(
				mUbos[i].getVkBuffer(), // buffer
				0, sizeof(UBO) // offset and range
			);

			vk::DescriptorImageInfo imgInfo(
				mTexSampler,
				mTexture.getVkImageview(),
				vk::ImageLayout::eShaderReadOnlyOptimal
			);

			std::array< vk::WriteDescriptorSet, 2> writeDesc;

			writeDesc[0] = vk::WriteDescriptorSet(
				mDescriptorSets[i],
				0,	// dst binding
				0,  // dst array element
				1,	// descriptor count
				vk::DescriptorType::eUniformBuffer,
				nullptr, // descriptor image,
				&buffInfo,// descriptor buffer
				nullptr // texel buffer view
			);
			writeDesc[1] = vk::WriteDescriptorSet(
				mDescriptorSets[i],
				1,	// dst binding
				0,  // dst array element
				1,	// descriptor count
				vk::DescriptorType::eCombinedImageSampler,
				&imgInfo, // descriptor image,
				nullptr,// descriptor buffer
				nullptr // texel buffer view
			);

			mContext.getDevice().updateDescriptorSets(
				static_cast<uint32_t>(writeDesc.size()),
					writeDesc.data(),	// write descriptions
				0, nullptr		// copy descriptions
			);
		}
	}

	void Engine::createPipelineLayout()
	{
		vk::PipelineLayoutCreateInfo createInfo(
			{}, // flags
			1, &mDescriptorSetLayout, // set layouts
			0, nullptr // push constants
		);

		mPipLayout = mContext.getDevice().createPipelineLayout(createInfo);
	}

	void Engine::createGraphicsPipeline()
	{
		vkg::GraphicsPipelineBuilder builder;
		
		// Set Vertex Input descriptions
		{
			vkg::VertexInputDescription vid;
			vid.addBinding(0, sizeof(Vertex))
				.addAttribute(0, 2, offsetof(Vertex, Vertex::pos))
				.addAttribute(1, 3, offsetof(Vertex, Vertex::color))
				.addAttribute(2, 2, offsetof(Vertex, Vertex::texCoord));

			assert(vid.getBindingDescription().size() == 1);
			builder.setVertexBindingDescriptions( vid.getBindingDescription() );
			builder.setVertexAttirbuteDescriptions( vid.getAttributeDescriptions() );
		}
		builder.setShaderStages(mShaderModules[0], mShaderModules[1]);
		builder.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
		builder.setViewportSize(mSwapChain.getExtent());
		builder.setMultisampleCount(vk::SampleCountFlagBits::e1);
		builder.setColorBlendAttachmentStd();
		builder.setPipelineLayout(mPipLayout);

		mGraphicsPipeline = builder.createPipeline(mContext.getDevice(), mRenderPass, 0);
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

	void Engine::createBuffers()
	{
		size_t VBsize = sizeof(vertices[0]) * vertices.size();
		size_t IBsize = sizeof(indices[0]) * indices.size();
		mVertexBuffer = mContext.createVertexBuffer(VBsize);
		mIndexBuffer = mContext.createIndexBuffer(IBsize);
		vkg::Buffer stageBuffer = mContext.createStagingBuffer(VBsize + IBsize);

		std::array<const void*, 2> datas = { vertices.data(), indices.data() };
		std::array<size_t, 2> sizes = { VBsize, IBsize };
		mContext.transferDataToGPU(stageBuffer, 2, datas.data(), sizes.data());

		vk::CommandBuffer copyCommand = mContext.getTransferTransientCommandPool().createCommandBuffer();

		copyCommand.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

		vk::BufferCopy buffercopy(
			0, 0, // src and dst offsets
			VBsize // bytes to copy
		);
		copyCommand.copyBuffer(stageBuffer.getVkBuffer(), mVertexBuffer.getVkBuffer(), 1, &buffercopy);
		buffercopy = vk::BufferCopy(
			VBsize, 0, // src and dst offsets
			IBsize // bytes to copy
		);
		copyCommand.copyBuffer(stageBuffer.getVkBuffer(), mIndexBuffer.getVkBuffer(), 1, &buffercopy);

		copyCommand.end();

		vk::Fence fence = mContext.createFence(false);
		mContext.getTransferTransientCommandPool().submitCommandBuffer(copyCommand, nullptr, {}, nullptr, fence);
		vk::Result res = mContext.getDevice().waitForFences(1, &fence, true, UINT64_MAX);
		assert(res == vk::Result::eSuccess);
		mContext.destroy(fence);
		mContext.safeDestroyBuffer(stageBuffer);
		mContext.getTransferTransientCommandPool().free(&copyCommand, 1);

	}

	void Engine::createUniformBuffers()
	{
		// Create uniform buffer objects
		mUbos.resize(mSwapChain.getNumImages());
		for (uint32_t i = 0; i < mSwapChain.getNumImages(); ++i) {
			mUbos[i] = mContext.createUniformBuffer(sizeof(UBO));
		}
	}

	void Engine::createTextureImage()
	{
		int32_t width, height, chann;
		stbi_uc* pix = stbi_load(
			"resources/textures/A.png",
			&width, &height, &chann, STBI_rgb_alpha);

		vk::DeviceSize imSize = 4 * width * height;

		if (!pix) {
			throw std::runtime_error("Error: Image can't be loaded!!");
		}

		// Transfer to GPU
		vkg::Buffer staging = mContext.createStagingBuffer(imSize);
		mContext.transferDataToGPU(staging, pix, imSize);

		stbi_image_free(pix);

		mTexture = mContext.createTexture2D(
			{ static_cast<vk::DeviceSize>(width),
				static_cast<vk::DeviceSize>(height)}, // extent
			1, vk::SampleCountFlagBits::e1, // mip levels and samples
			vk::Format::eR8G8B8A8Srgb,
			vk::ImageAspectFlagBits::eColor
		);

		vk::CommandBuffer cmd = mContext.getTransferTransientCommandPool().createCommandBuffer();

		cmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

		// First transition image to dstOptimal
		vk::ImageMemoryBarrier barrier(
			vk::AccessFlags{}, // src AccessMask
			vk::AccessFlagBits::eTransferWrite, // dst AccessMask
			vk::ImageLayout::eUndefined,// old layout
			vk::ImageLayout::eTransferDstOptimal,// new layout
			VK_QUEUE_FAMILY_IGNORED,	// src queue family
			VK_QUEUE_FAMILY_IGNORED,	// dst queue family
			mTexture.getVkImage(),	// image
			vk::ImageSubresourceRange( // range
				vk::ImageAspectFlagBits::eColor,
				0, 1, 0, 1 // base mip, level count, base array, layer count
			)
		);

		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe, // src stage mask
			vk::PipelineStageFlagBits::eTransfer, // dst stage mask
			vk::DependencyFlagBits{},
			0, nullptr, 0, nullptr, // buffer and memory barrier
			1, &barrier				// image memory barrier
		);

		// Copy into image

		vk::BufferImageCopy regionInfo(
			0, 0u, 0u,	// offset, row length, image height
			vk::ImageSubresourceLayers(
				vk::ImageAspectFlagBits::eColor,
				0, 0, 1 // mip level, base array, layer count
			),		// subresource range
			vk::Offset3D(0),
			vk::Extent3D{ mTexture.getExtent(), 1 }
		);

		cmd.copyBufferToImage(
			staging.getVkBuffer(), mTexture.getVkImage(),	// src and dst buffer / image
			vk::ImageLayout::eTransferDstOptimal,			// dst image layout
			1, &regionInfo									// regions
		);

		// Transition into shader access
		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
		barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
		barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		barrier.setSrcQueueFamilyIndex(mContext.getTransferFamilyIdx());
		barrier.setDstQueueFamilyIndex(mContext.getGraphicsFamilyIdx());

		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer, // src stage mask
			vk::PipelineStageFlagBits::eFragmentShader, // dst stage mask
			vk::DependencyFlagBits{},
			0, nullptr, 0, nullptr, // buffer and memory barrier
			1, &barrier				// image memory barrier
		);

		cmd.end();


		vk::Fence fence = mContext.createFence(false);
		mContext.getTransferTransientCommandPool().submitCommandBuffer(cmd, nullptr, {}, nullptr, fence);

		// create sampler
		mTexSampler = mContext.createSampler(vk::SamplerAddressMode::eRepeat);

		vk::Result res = mContext.getDevice().waitForFences(1, &fence, true, UINT64_MAX);
		assert(res == vk::Result::eSuccess);
		mContext.destroy(fence);

		mContext.safeDestroyBuffer(staging);
		mContext.getTransferTransientCommandPool().free(&cmd, 1);

		// because of ownership transfer we need to do transition on graphics
		cmd = mContext.getGraphicsCommandPool().createCommandBuffer();
		cmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer, // src stage mask
			vk::PipelineStageFlagBits::eFragmentShader, // dst stage mask
			vk::DependencyFlagBits{},
			0, nullptr, 0, nullptr, // buffer and memory barrier
			1, &barrier				// image memory barrier
		);
		cmd.end();
		mContext.getGraphicsCommandPool().submitCommandBuffer(cmd, nullptr, {}, nullptr);
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
		
		// Destroy Buffers
		mContext.safeDestroyBuffer(mVertexBuffer);
		mContext.safeDestroyBuffer(mIndexBuffer);
		
		mContext.safeDestroyImage(mTexture);

		mContext.destroy(mTexSampler);

		// layout
		mContext.getDevice().destroyDescriptorSetLayout(mDescriptorSetLayout);

		mContext.getDevice().destroyPipelineLayout(mPipLayout);

		mContext.destroy(mShaderModules[0]);
		mContext.destroy(mShaderModules[1]);

		mDescriptorManager.destroy(mContext);
	}

	void Engine::cleanupSwapChainDependantObjs()
	{
		for (vk::Framebuffer frambuffer : mPresentFramebuffers) {
			mContext.destroy(frambuffer);
		}

		for (vkg::Buffer& b : mUbos) {
			mContext.safeDestroyBuffer(b);
		}

		// free descriptor sets
		for (vk::DescriptorSet set : mDescriptorSets) {
			mDescriptorManager.freeDescriptorSet(set, mDescriptorSetLayout);
		}

		mContext.getDevice().destroyPipeline(mGraphicsPipeline);
		mContext.destroy(mRenderPass);
	}

}; // namespace gr