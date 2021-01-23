#pragma once

#include "control/FrameContext.h"
#include "graphics/present/SwapChain.h"
#include "graphics/Window.h"
#include "graphics/render/RenderPass.h"
#include "graphics/resources/DescriptorManager.h"

namespace gr
{

	class Engine
	{
	public:
		static void init();


		static void terminate();


		void run();

	protected:
		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

		std::array<FrameContext, MAX_FRAMES_IN_FLIGHT> mContexts;
		std::unique_ptr<vkg::RenderContext> pRenderContext;
		vkg::SwapChain mSwapChain;
		vkg::Window mWindow = vkg::Window(1280, 1024, "Test");


		std::vector<vk::Framebuffer> mPresentFramebuffers;
		vkg::RenderPass mRenderPass;

		vk::PipelineLayout mPipLayout;
		vk::ShaderModule mShaderModules[2];
		vk::Pipeline mGraphicsPipeline;

		uint32_t mCurrentFrame = 0;
		std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> mImageAvailableSemaphores;
		std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> mRenderingFinishedSemaphores;
		std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> mInFlightFences;
		std::vector<vk::Fence> mImagesInFlightFences;

		vkg::Buffer mVertexBuffer;
		vkg::Buffer mIndexBuffer;
		std::vector<vkg::Buffer> mUbos;
		vkg::Image2D mTexture;
		vk::Sampler mTexSampler;

		vkg::DescriptorManager mDescriptorManager;
		vk::DescriptorSetLayout mDescriptorSetLayout;
		std::vector<vk::DescriptorSet> mDescriptorSets;

		void draw(FrameContext& frameContext);

		void updateUBO(const FrameContext& frameContext, uint32_t currentImage);

		void createRenderPass();
		void recreateSwapChain();

		vk::CommandBuffer createAndRecordGraphicCommandBuffers(FrameContext* frame);


		void createShaderModules();

		void createDescriptorSetLayout();
		void createDescriptorSets();

		void createPipelineLayout();

		void createGraphicsPipeline();

		void createSyncObjects();


		void createBuffers();
		void createUniformBuffers();
		void createTextureImage();

		void cleanup();
		void cleanupSwapChainDependantObjs();
	};

}; // namespace gr