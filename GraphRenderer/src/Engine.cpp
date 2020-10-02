#include "Engine.h"

#ifdef _WIN32 // TO avoid APIENTRY redefinition warning
#include <Windows.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "graphics/Window.h"
#include "graphics/AppInstance.h"
#include "graphics/DeviceComp.h"
#include "graphics/memory/MemoryManager.h"
#include "graphics/present/SwapChain.h"
#include "graphics/Admin.h"


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

		Window window(800, 600, "Test");
		AppInstance instance;
		window.createVkSurface(instance);

		DeviceComp device(instance, true, &window.getSurface());

		MemoryManager memManager(instance.getInstance(), device, device);

		Admin admin(std::move(device), std::move(memManager));

		SwapChain swapChain(device, window);

		Image2D image = admin.createDeviceImage2D({ 800,600 },
			1,
			vk::SampleCountFlagBits::e1,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eSampled);

		while (!window.windowShouldClose()) {
			Window::pollEvents();
		}

		admin.destroyImage(image);

		swapChain.destroy(device);
		admin.destroy();
		window.destroy(instance);
		instance.destroy();
	}

}; // namespace gr