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
#include "control/FrameContext.h"

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

		pRenderContext = std::make_unique<vkg::RenderContext>();
		pRenderContext->createInstance({}, true);

		mWindow.createVkSurface(pRenderContext->getInstance());

		pRenderContext->createDevice(true, &mWindow.getSurface());

		mSwapChain = SwapChain(*pRenderContext, mWindow);

		createSyncObjects();

		/*for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			mContexts[i] = FrameContext(i, pRenderContext.get());
			mContexts[i].recreateCommandPools();
			mContexts[i].commandFlusher().setNumControls(*pRenderContext, mSwapChain.getNumImages());
		}*/
		{
			std::vector<FrameContext> v = FrameContext::createContexts(MAX_FRAMES_IN_FLIGHT, pRenderContext.get());
			std::copy_n(std::make_move_iterator(v.begin()), MAX_FRAMES_IN_FLIGHT, mContexts.begin());
			mCommandFlusherGraphicsBlock = pRenderContext->getCommandFlusher()->createNewBlock(CommandFlusher::Type::eGRAPHICS);
		}

		createRenderPass();
		mPresentFramebuffers = mSwapChain.createFramebuffersOfSwapImages(mRenderPass);

		mDescriptorManager.initialize(*pRenderContext);

		std::array<grjob::Job, 5> jobs;
		jobs[0] = grjob::Job(&Engine::createShaderModules, this);
		jobs[1] = grjob::Job(&Engine::createBuffers, this);
		jobs[2] = grjob::Job(&Engine::createUniformBuffers, this);
		jobs[3] = grjob::Job(&Engine::createTextureImage, this);
		jobs[4] = grjob::Job(&Engine::createDescriptorSetLayout, this);

		grjob::Counter* counter = nullptr;
		grjob::runJobBatch(grjob::Priority::eMid, jobs.data(), static_cast<uint32_t>(jobs.size()), &counter);
		grjob::waitForCounterAndFree(counter, 0);

		createDescriptorSets();
		createPipelineLayout();
		createGraphicsPipeline();

		pRenderContext->waitIdle();

		// init to start with frame zero
		mCurrentFrame = MAX_FRAMES_IN_FLIGHT - 1;

		while (!mWindow.windowShouldClose()) {
			// advance frame
			mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
			mContexts[mCurrentFrame].advanceFrameCount();

			// Wait for current frame, if it was still being executed
			{
				//vk::Result res2 = pRenderContext->getDevice().waitForFences(1, mInFlightFences.data() + mCurrentFrame, true, UINT64_MAX);
				
				uint64_t waitValue = mContexts[mCurrentFrame].getFrameCount();
				vk::SemaphoreWaitInfo waitInfo(
					vk::SemaphoreWaitFlags{},
					1, &mFrameAvailableTimelineSemaphore,
					&waitValue
				);
				vk::Result res = pRenderContext->getDevice().waitSemaphores(waitInfo, UINT64_MAX);
				
				assert(res == vk::Result::eSuccess);
			}

			mContexts[mCurrentFrame].updateTime(glfwGetTime());
			mContexts[mCurrentFrame].resetFrameResources();

			draw(mContexts[mCurrentFrame]);

			Window::pollEvents();
		}

		// Destroy everything

		pRenderContext->waitIdle();


		cleanup();

		for (FrameContext& c : mContexts) {
			c.destroy();
		}

		mSwapChain.destroy();
		mWindow.destroy(pRenderContext->getInstance());
		pRenderContext->destroy();
	}

	void Engine::draw(FrameContext& frameContext)
	{
		vk::Result res;

		const vkg::ResetCommandPool& cmdPool = frameContext.graphicsPool();

		uint32_t imageIdx;
		bool outOfDateSwapChain = !mSwapChain.acquireNextImageBlock(
			mImageAvailableSemaphores[frameContext.getIdx()], &imageIdx);

		frameContext.setImageIdx(imageIdx);

		if (outOfDateSwapChain) {
			recreateSwapChain();
			return;
		}

		frameContext.rc().getCommandFlusher()->pushGraphicsCB(mCommandFlusherGraphicsBlock, createAndRecordGraphicCommandBuffers(mContexts.data() + frameContext.getIdx()));
		frameContext.rc().getCommandFlusher()->pushWait(vkg::CommandFlusher::Type::eGRAPHICS, mCommandFlusherGraphicsBlock,
			mImageAvailableSemaphores[frameContext.getIdx()], vk::PipelineStageFlagBits::eColorAttachmentOutput);
		frameContext.rc().getCommandFlusher()->pushSignal(vkg::CommandFlusher::Type::eGRAPHICS, mCommandFlusherGraphicsBlock,
			mRenderingFinishedSemaphores[frameContext.getIdx()]);
		frameContext.rc().getCommandFlusher()->pushSignal(vkg::CommandFlusher::Type::eGRAPHICS, mCommandFlusherGraphicsBlock,
			mFrameAvailableTimelineSemaphore, frameContext.getNextFrameCount());


		// maybe out of order next image, thus wait also for next image 
		{
			vk::SemaphoreWaitInfo waitInfo(
				vk::SemaphoreWaitFlags{}, 1,
				&mFrameAvailableTimelineSemaphore,		// semaphore to wait
				&mInFlightSemaphoreValues[imageIdx]);	// value to wait

			res = pRenderContext->getDevice().waitSemaphores(waitInfo, UINT64_MAX);
			assert(res == vk::Result::eSuccess);
			// update in flight image
			mInFlightSemaphoreValues[imageIdx] = frameContext.getNextFrameCount();
		}

		updateUBO(frameContext, frameContext.getIdx());
		
		frameContext.rc().getCommandFlusher()->flush();

		bool swapChainNeedsRecreation = 
			!frameContext.presentPool().submitPresentationImage(
				mSwapChain.getVkSwapChain(),
				imageIdx,
				&mRenderingFinishedSemaphores[frameContext.getIdx()]
			);

		if (swapChainNeedsRecreation) {
			recreateSwapChain();
		}
	}

	void Engine::updateUBO(const FrameContext& frameContext, uint32_t currentImage)
	{
		
		float time = frameContext.timef();
		UBO ubo;
		ubo.M = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
			glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.V = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f,
			0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.P = glm::perspective(glm::radians(45.0f),
			mSwapChain.getExtent().width / static_cast<float>(mSwapChain.getExtent().height),
			0.1f, 10.0f);
		ubo.P[1][1] *= -1.0;

		pRenderContext->transferDataToGPU(mUbos[currentImage], &ubo, sizeof(ubo));
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

		mRenderPass =  builder.buildRenderPass(*pRenderContext);
	}



	void Engine::recreateSwapChain()
	{
		while (mWindow.isWindowMinimized()) {
			mWindow.waitEvents();
		}

		pRenderContext->waitIdle();

		cleanupSwapChainDependantObjs();

		mSwapChain.recreateSwapChain(*pRenderContext, mWindow);

		createRenderPass();

		mPresentFramebuffers = mSwapChain.createFramebuffersOfSwapImages(mRenderPass);

		createUniformBuffers();
		createDescriptorSets();
		createGraphicsPipeline();
	}

	vk::CommandBuffer Engine::createAndRecordGraphicCommandBuffers(FrameContext* frame)
	{
		 vkg::ResetCommandPool& cmdPool = frame->graphicsPool();

		vk::CommandBuffer buff = cmdPool.newCommandBuffer();


		const uint32_t graphicsQueueFamilyIdx = pRenderContext->getGraphicsFamilyIdx();

		vk::CommandBufferBeginInfo beginInfo = {};

		buff.begin(beginInfo);

		std::array<vk::ClearValue, 1> clearVal = {};
		clearVal[0].color.setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });

		vk::RenderPassBeginInfo passInfo(
			mRenderPass,
			mPresentFramebuffers[frame->getImageIdx()],
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
			0, 1, &mDescriptorSets[frame->getIdx()],	// first set, descriptors sets
			0, nullptr					// dynamic offsets
		);

		buff.drawIndexed(
			static_cast<uint32_t>(indices.size()), // index count
			1, 0, 0, 0	// instance count, and offsets
		);

		buff.endRenderPass();

		buff.end();

		return buff;

	}

	void Engine::createShaderModules()
	{
		{
			grjob::Job jobs[2];
			jobs[0] = grjob::Job(&vkg::RenderContext::createShaderModule, pRenderContext.get(), "resources/shaders/SPIR-V/sampler.vert.spv", mShaderModules + 0);
			jobs[1] = grjob::Job(&vkg::RenderContext::createShaderModule, pRenderContext.get(), "resources/shaders/SPIR-V/sampler.frag.spv", mShaderModules + 1);
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

		mDescriptorSetLayout = pRenderContext->getDevice().createDescriptorSetLayout(createInfo);

	}

	void Engine::createDescriptorSets()
	{
		mDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		mDescriptorManager.allocateDescriptorSets(
			*pRenderContext,
			MAX_FRAMES_IN_FLIGHT,
			mDescriptorSetLayout,
			mDescriptorSets.data()
		);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
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

			pRenderContext->getDevice().updateDescriptorSets(
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

		mPipLayout = pRenderContext->getDevice().createPipelineLayout(createInfo);
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

		mGraphicsPipeline = builder.createPipeline(pRenderContext->getDevice(), mRenderPass, 0);
	}

	void Engine::createSyncObjects()
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			mImageAvailableSemaphores[i] = pRenderContext->createSemaphore();
			mRenderingFinishedSemaphores[i] = pRenderContext->createSemaphore();
		}

		mInFlightSemaphoreValues.fill(0);
		mImagesInFlightFences.resize(mSwapChain.getNumImages());

		mFrameAvailableTimelineSemaphore = pRenderContext->createTimelineSemaphore(2 * mContexts.size() - 1);
	}

	void Engine::createBuffers()
	{
		size_t VBsize = sizeof(vertices[0]) * vertices.size();
		size_t IBsize = sizeof(indices[0]) * indices.size();
		mVertexBuffer = pRenderContext->createVertexBuffer(VBsize);
		mIndexBuffer = pRenderContext->createIndexBuffer(IBsize);
		vkg::Buffer stageBuffer = pRenderContext->createStagingBuffer(VBsize + IBsize);

		std::array<const void*, 2> datas = { vertices.data(), indices.data() };
		std::array<size_t, 2> sizes = { VBsize, IBsize };
		pRenderContext->transferDataToGPU(stageBuffer, 2, datas.data(), sizes.data());

		vk::CommandBuffer copyCommand = mContexts.front().transferPool().newCommandBuffer();

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

		vk::Fence fence = pRenderContext->createFence(false);
		mContexts.front().transferPool().submitCommandBuffer(copyCommand, nullptr, {}, nullptr, fence);
		vk::Result res = pRenderContext->getDevice().waitForFences(1, &fence, true, UINT64_MAX);
		assert(res == vk::Result::eSuccess);
		pRenderContext->destroy(fence);
		pRenderContext->safeDestroyBuffer(stageBuffer);

	}

	void Engine::createUniformBuffers()
	{
		// Create uniform buffer objects
		mUbos.resize(MAX_FRAMES_IN_FLIGHT);
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			mUbos[i] = pRenderContext->createUniformBuffer(sizeof(UBO));
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
		vkg::Buffer staging = pRenderContext->createStagingBuffer(imSize);
		pRenderContext->transferDataToGPU(staging, pix, imSize);

		stbi_image_free(pix);

		mTexture = pRenderContext->createTexture2D(
			{ static_cast<vk::DeviceSize>(width),
				static_cast<vk::DeviceSize>(height)}, // extent
			1, vk::SampleCountFlagBits::e1, // mip levels and samples
			vk::Format::eR8G8B8A8Srgb,
			vk::ImageAspectFlagBits::eColor
		);

		vk::CommandBuffer cmd = mContexts.front().transferPool().newCommandBuffer();

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
		barrier.setSrcQueueFamilyIndex(pRenderContext->getTransferFamilyIdx());
		barrier.setDstQueueFamilyIndex(pRenderContext->getGraphicsFamilyIdx());

		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer, // src stage mask
			vk::PipelineStageFlagBits::eFragmentShader, // dst stage mask
			vk::DependencyFlagBits{},
			0, nullptr, 0, nullptr, // buffer and memory barrier
			1, &barrier				// image memory barrier
		);

		cmd.end();


		vk::Fence fence = pRenderContext->createFence(false);
		mContexts.front().transferPool().submitCommandBuffer(cmd, nullptr, {}, nullptr, fence);

		// create sampler
		mTexSampler = pRenderContext->createSampler(vk::SamplerAddressMode::eRepeat);

		vk::Result res = pRenderContext->getDevice().waitForFences(1, &fence, true, UINT64_MAX);
		assert(res == vk::Result::eSuccess);
		pRenderContext->destroy(fence);

		pRenderContext->safeDestroyBuffer(staging);

		// because of ownership transfer we need to do transition on graphics
		cmd = mContexts.front().graphicsPool().newCommandBuffer();
		cmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer, // src stage mask
			vk::PipelineStageFlagBits::eFragmentShader, // dst stage mask
			vk::DependencyFlagBits{},
			0, nullptr, 0, nullptr, // buffer and memory barrier
			1, &barrier				// image memory barrier
		);
		cmd.end();
		mContexts.front().graphicsPool().submitCommandBuffer(cmd, nullptr, {}, nullptr);
	}

	void Engine::cleanup()
	{
		// destroy sync objects
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			pRenderContext->destroy(mImageAvailableSemaphores[i]);
			pRenderContext->destroy(mRenderingFinishedSemaphores[i]);
		}
		pRenderContext->destroy(mFrameAvailableTimelineSemaphore);

		cleanupSwapChainDependantObjs();
		
		// Destroy Buffers
		pRenderContext->safeDestroyBuffer(mVertexBuffer);
		pRenderContext->safeDestroyBuffer(mIndexBuffer);
		
		pRenderContext->safeDestroyImage(mTexture);

		pRenderContext->destroy(mTexSampler);

		// layout
		pRenderContext->getDevice().destroyDescriptorSetLayout(mDescriptorSetLayout);

		pRenderContext->getDevice().destroyPipelineLayout(mPipLayout);

		pRenderContext->destroy(mShaderModules[0]);
		pRenderContext->destroy(mShaderModules[1]);

		mDescriptorManager.destroy(*pRenderContext);
	}

	void Engine::cleanupSwapChainDependantObjs()
	{
		for (vk::Framebuffer frambuffer : mPresentFramebuffers) {
			pRenderContext->destroy(frambuffer);
		}

		for (vkg::Buffer& b : mUbos) {
			pRenderContext->safeDestroyBuffer(b);
		}

		// free descriptor sets
		for (vk::DescriptorSet set : mDescriptorSets) {
			mDescriptorManager.freeDescriptorSet(set, mDescriptorSetLayout);
		}

		pRenderContext->getDevice().destroyPipeline(mGraphicsPipeline);
		pRenderContext->destroy(mRenderPass);
	}

}; // namespace gr