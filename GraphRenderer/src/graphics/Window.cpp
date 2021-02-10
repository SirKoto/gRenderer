#include "Window.h"
#include <GLFW/glfw3.h>

#include "DebugVk.h"

namespace gr
{
namespace vkg
{

void Window::initialize(int width, int heigth, const std::string& windowTitle)
{
	assert(width > 0 && heigth > 0);
	mWidth = width;
	mHeight = heigth;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mWindow = glfwCreateWindow(width, heigth, windowTitle.c_str(), nullptr, nullptr);
}





void Window::createVkSurface(const vk::Instance& instance)
{
	VkSurfaceKHR surfRaw;
	VkResult res = glfwCreateWindowSurface(
		instance,
		reinterpret_cast<GLFWwindow*>( mWindow ),
		nullptr,
		&surfRaw);
	mSurface = vk::createResultValue(static_cast<vk::Result>(res), surfRaw, "Error: Failed to create VkSurface");
}

bool Window::windowShouldClose() const
{
	return glfwWindowShouldClose(reinterpret_cast<GLFWwindow*>(mWindow));
}


bool Window::isWindowMinimized() const
{
	int w, h;
	glfwGetFramebufferSize(reinterpret_cast<GLFWwindow*>(mWindow), &w, &h);

	return w == 0 || h == 0;
}

void Window::destroy(const vk::Instance& instance)
{
	if (mSurface != VK_NULL_HANDLE) {
		instance.destroySurfaceKHR(mSurface, nullptr);
	}
	glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(mWindow));
}

void Window::pollEvents()
{
	glfwPollEvents();
}

void Window::waitEvents()
{
	glfwWaitEvents();
}

std::vector<const char*> Window::getWindowVkExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (DebugVk::validationLayersVkEnabled)
	{
		extensions.push_back(DebugVk::debugExtensionName);
	}

	return extensions;
}

}; // namespace vkg
}; // namespace gr