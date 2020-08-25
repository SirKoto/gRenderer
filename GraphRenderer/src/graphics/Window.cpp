#include "Window.h"
#include <GLFW/glfw3.h>

#include "DebugVk.h"


Window::Window(int width, int heigth, const std::string& windowTitle) : 
	mWidth(width), mHeight(heigth)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mWindow = glfwCreateWindow(width, heigth, windowTitle.c_str(), nullptr, nullptr);
}

void Window::createVkSurface(const vk::Instance& instance, vk::SurfaceKHR* surface) const
{
	VkSurfaceKHR surfRaw;
	VkResult res = glfwCreateWindowSurface(
		instance,
		reinterpret_cast<GLFWwindow*>( mWindow ),
		nullptr,
		&surfRaw);
	*surface = vk::createResultValue(static_cast<vk::Result>(res), surfRaw, "Error: Failed to create VkSurface");
}

bool Window::windowShouldClose() const
{
	return glfwWindowShouldClose(reinterpret_cast<GLFWwindow*>(mWindow));
}

void Window::pollEvents()
{
	glfwPollEvents();
}

std::vector<const char*> Window::getWindowVkExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (DebugVk::validationLayersVkEnabled)
	{
		extensions.push_back(DebugVk::getVkDebugExtensionName());
	}

	return extensions;
}
