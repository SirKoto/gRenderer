#include "Engine.h"

#ifdef _WIN32 // TO avoid APIENTRY redefinition warning
#include <Windows.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "graphics/Window.h"
#include "graphics/AppInstance.h"
#include "graphics/DeviceComp.h"

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
	window.createVkSurface(app);

	DeviceComp device(app, true, true, &window.getSurface());
	

	while (!window.windowShouldClose()) {
		Window::pollEvents();
	}

	device.destroy();
	window.destroy(app);
	app.destroy();
}
