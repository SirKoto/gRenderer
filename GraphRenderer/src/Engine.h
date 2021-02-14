#pragma once

#include "control/FrameContext.h"
#include "graphics/present/SwapChain.h"
#include "graphics/render/RenderPass.h"
#include "gui/Gui.h"

namespace gr
{

	class Engine
	{
	public:
		static void init();


		static void terminate();


		void run();

	protected:
		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

		GlobalContext mGlobalContext;

		std::array<FrameContext, MAX_FRAMES_IN_FLIGHT> mContexts;
		
		vkg::SwapChain mSwapChain;

		Gui mGui;

		std::vector<vk::Framebuffer> mPresentFramebuffers;
		vkg::Image2D mColorImage, mDepthImage;
		vkg::RenderPass mRenderPass;

		vk::PipelineLayout mPipLayout;
		vk::ShaderModule mShaderModules[2];
		vk::Pipeline mGraphicsPipeline, mWireframePipeline;

		uint32_t mCurrentFrame = 0;
		vk::Semaphore mFrameAvailableTimelineSemaphore;
		std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> mImageAvailableSemaphores;
		std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> mRenderingFinishedSemaphores;
		std::array<uint64_t, MAX_FRAMES_IN_FLIGHT> mInFlightSemaphoreValues;
		std::vector<vk::Fence> mImagesInFlightFences;

		vkg::Buffer mVertexBuffer;
		vkg::Buffer mIndexBuffer;
		std::vector<vkg::Buffer> mUbos;
		vkg::Image2D mTexture;
		vk::Sampler mTexSampler;

		vk::DescriptorSetLayout mDescriptorSetLayout;
		std::vector<vk::DescriptorSet> mDescriptorSets;

		uint32_t mCommandFlusherGraphicsBlock;

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

		void createFrameBufferObjects();

		void createBuffers();
		void createUniformBuffers();
		void createTextureImage();

		void cleanup();
		void cleanupSwapChainDependantObjs();
	};

}; // namespace gr