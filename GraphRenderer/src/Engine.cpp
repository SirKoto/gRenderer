#include "Engine.h"

#ifdef _WIN32 // TO avoid APIENTRY redefinition warning
#include <Windows.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "graphics/Window.h"
#include "graphics/AppInstance.h"

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
	Window window(800, 600, "Test");
	AppInstance app;

	while (!window.windowShouldClose()) {
		Window::pollEvents();
	}
}
