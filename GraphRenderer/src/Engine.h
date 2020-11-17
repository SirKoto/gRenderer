#pragma once

#include "graphics/Admin.h"
#include "graphics/present/SwapChain.h"
#include "graphics/Window.h"
#include "graphics/render/RenderPass.h"

namespace gr
{

	class Engine
	{
	public:
		static void init();


		static void terminate();

		void run();

	protected:
		vkg::Admin mAdmin;
		vkg::SwapChain mSwapChain;
		vkg::Window mWindow = vkg::Window(800, 600, "Test");


		std::vector<vk::CommandBuffer> mPresentCommandBuffers;
		std::vector<vk::Framebuffer> mPresentFramebuffers;
		vkg::RenderPass mRenderPass;

		vk::Semaphore mImageAvailableSemaphore;
		vk::Semaphore mRenderingFinishedSemaphore;

		void draw();

		void createRenderPass();
		void createAndRecordPresentCommandBuffers();
		void deletePresentCommandBuffers();
		void recreateSwapChain();
	};

}; // namespace gr