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

		vkg::RenderContext* pRenderContext = &mGlobalContext.rc();
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
			std::vector<FrameContext> v = FrameContext::createContexts(MAX_FRAMES_IN_FLIGHT, &mGlobalContext);
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

			pRenderContext->flushData();

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

			res = mGlobalContext.rc().getDevice().waitSemaphores(waitInfo, UINT64_MAX);
			assert(res == vk::Result::eSuccess);
			// update in flight image
			mInFlightSemaphoreValues[imageIdx] = frameContext.getNextFrameCount();
		}

		updateUBO(frameContext, frameContext.getIdx());

		// Push wait for transfers
		{
			vk::Semaphore sem;
			uint64_t value;
			mGlobalContext.rc().getTransferer()->updateAndFlushTransfers(
				&mGlobalContext.rc(), &sem, &value);
			frameContext.rc().getCommandFlusher()->pushWait(vkg::CommandFlusher::Type::eGRAPHICS, mCommandFlusherGraphicsBlock,
				sem, vk::PipelineStageFlagBits::eTopOfPipe, value);
		}
		
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

		mGlobalContext.rc().transferDataToGPU(mUbos[currentImage], &ubo, sizeof(ubo));
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

		mRenderPass =  builder.buildRenderPass(mGlobalContext.rc());
	}



	void Engine::recreateSwapChain()
	{
		while (mWindow.isWindowMinimized()) {
			mWindow.waitEvents();
		}

		mGlobalContext.rc().waitIdle();

		cleanupSwapChainDependantObjs();

		mSwapChain.recreateSwapChain(mGlobalContext.rc(), mWindow);

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


		const uint32_t graphicsQueueFamilyIdx = mGlobalContext.rc().getGraphicsFamilyIdx();

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
			jobs[0] = grjob::Job(&vkg::RenderContext::createShaderModule, &mGlobalContext.rc(), "resources/shaders/SPIR-V/sampler.vert.spv", mShaderModules + 0);
			jobs[1] = grjob::Job(&vkg::RenderContext::createShaderModule, &mGlobalContext.rc(), "resources/shaders/SPIR-V/sampler.frag.spv", mShaderModules + 1);
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

		mDescriptorSetLayout = mGlobalContext.rc().getDevice().createDescriptorSetLayout(createInfo);

	}

	void Engine::createDescriptorSets()
	{
		mDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		mDescriptorManager.allocateDescriptorSets(
			mGlobalContext.rc(),
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

			mGlobalContext.rc().getDevice().updateDescriptorSets(
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

		mPipLayout = mGlobalContext.rc().getDevice().createPipelineLayout(createInfo);
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

		mGraphicsPipeline = builder.createPipeline(mGlobalContext.rc().getDevice(), mRenderPass, 0);
	}

	void Engine::createSyncObjects()
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			mImageAvailableSemaphores[i] = mGlobalContext.rc().createSemaphore();
			mRenderingFinishedSemaphores[i] = mGlobalContext.rc().createSemaphore();
		}

		mInFlightSemaphoreValues.fill(0);
		mImagesInFlightFences.resize(mSwapChain.getNumImages());

		mFrameAvailableTimelineSemaphore = mGlobalContext.rc().createTimelineSemaphore(2 * mContexts.size() - 1);
	}

	void Engine::createBuffers()
	{
		size_t VBsize = sizeof(vertices[0]) * vertices.size();
		size_t IBsize = sizeof(indices[0]) * indices.size();
		mVertexBuffer = mGlobalContext.rc().createVertexBuffer(VBsize);
		mIndexBuffer = mGlobalContext.rc().createIndexBuffer(IBsize);

		mGlobalContext.rc().getTransferer()->transferToBuffer(mGlobalContext.rc(),
			vertices.data(), VBsize, mVertexBuffer);
		mGlobalContext.rc().getTransferer()->transferToBuffer(mGlobalContext.rc(),
			indices.data(), IBsize, mIndexBuffer);

	}

	void Engine::createUniformBuffers()
	{
		// Create uniform buffer objects
		mUbos.resize(MAX_FRAMES_IN_FLIGHT);
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			mUbos[i] = mGlobalContext.rc().createUniformBuffer(sizeof(UBO));
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


		mTexture = mGlobalContext.rc().createTexture2D(
			{ static_cast<vk::DeviceSize>(width),
				static_cast<vk::DeviceSize>(height)}, // extent
			1, vk::SampleCountFlagBits::e1, // mip levels and samples
			vk::Format::eR8G8B8A8Srgb,
			vk::ImageAspectFlagBits::eColor
		);

		mGlobalContext.rc().getTransferer()->transferToImage(
			mGlobalContext.rc(), pix,	// rc and data ptr
			imSize, mTexture,		// bytes, Image2D
			vk::ImageSubresourceLayers(
				vk::ImageAspectFlagBits::eColor,
				0, 0, 1 // mip level, base array, layer count
			),
			vk::AccessFlagBits::eShaderRead, // dst Access Mask
			vk::ImageLayout::eShaderReadOnlyOptimal, // dst Image Layout
			vk::PipelineStageFlagBits::eFragmentShader, // dstStage
			true
		);

		stbi_image_free(pix);

		// create sampler
		mTexSampler = mGlobalContext.rc().createSampler(vk::SamplerAddressMode::eRepeat);
	}

	void Engine::cleanup()
	{
		// destroy sync objects
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			mGlobalContext.rc().destroy(mImageAvailableSemaphores[i]);
			mGlobalContext.rc().destroy(mRenderingFinishedSemaphores[i]);
		}
		mGlobalContext.rc().destroy(mFrameAvailableTimelineSemaphore);

		cleanupSwapChainDependantObjs();
		
		// Destroy Buffers
		mGlobalContext.rc().safeDestroyBuffer(mVertexBuffer);
		mGlobalContext.rc().safeDestroyBuffer(mIndexBuffer);
		
		mGlobalContext.rc().safeDestroyImage(mTexture);

		mGlobalContext.rc().destroy(mTexSampler);

		// layout
		mGlobalContext.rc().getDevice().destroyDescriptorSetLayout(mDescriptorSetLayout);

		mGlobalContext.rc().getDevice().destroyPipelineLayout(mPipLayout);

		mGlobalContext.rc().destroy(mShaderModules[0]);
		mGlobalContext.rc().destroy(mShaderModules[1]);

		mDescriptorManager.destroy(mGlobalContext.rc());
	}

	void Engine::cleanupSwapChainDependantObjs()
	{
		for (vk::Framebuffer frambuffer : mPresentFramebuffers) {
			mGlobalContext.rc().destroy(frambuffer);
		}

		for (vkg::Buffer& b : mUbos) {
			mGlobalContext.rc().safeDestroyBuffer(b);
		}

		// free descriptor sets
		for (vk::DescriptorSet set : mDescriptorSets) {
			mDescriptorManager.freeDescriptorSet(set, mDescriptorSetLayout);
		}

		mGlobalContext.rc().getDevice().destroyPipeline(mGraphicsPipeline);
		mGlobalContext.rc().destroy(mRenderPass);
	}

}; // namespace gr