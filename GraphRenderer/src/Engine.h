#pragma once

#include "graphics/Context.h"
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
		vkg::Context mContext;
		vkg::SwapChain mSwapChain;
		vkg::Window mWindow = vkg::Window(1280, 1024, "Test");


		std::vector<vk::CommandBuffer> mPresentCommandBuffers;
		std::vector<vk::Framebuffer> mPresentFramebuffers;
		std::vector<vk::CommandBuffer> mGraphicCommandBuffers;
		vkg::RenderPass mRenderPass;

		vk::PipelineLayout mPipLayout;
		vk::ShaderModule mShaderModules[2];
		vk::Pipeline mGraphicsPipeline;

		uint32_t mCurrentFrame = 0;
		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
		std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> mImageAvailableSemaphores;
		std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> mRenderingFinishedSemaphores;
		std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> mInFlightFences;
		std::vector<vk::Fence> mImagesInFlightFences;

		vkg::Buffer mVertexBuffer;
		vkg::Buffer mIndexBuffer;
		std::vector<vkg::Buffer> mUbos;

		vkg::DescriptorManager mDescriptorManager;
		vk::DescriptorSetLayout mDescriptorSetLayout;
		std::vector<vk::DescriptorSet> mDescriptorSets;

		void draw();

		void updateUBO(uint32_t currentImage);

		void createRenderPass();
		void recreateSwapChain();

		void createAndRecordGraphicCommandBuffers();

		void createShaderModules();

		void createDescriptorSetLayout();
		void createDescriptorSets();

		void createPipelineLayout();

		void createGraphicsPipeline();

		void createSyncObjects();


		void createBuffers();
		void createUniformBuffers();

		void cleanup();
		void cleanupSwapChainDependantObjs();
	};

}; // namespace gr