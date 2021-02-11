#include "Window.h"
#include <GLFW/glfw3.h>

#include "DebugVk.h"

namespace gr
{
namespace vkg
{

void Window::s_mouseWheelCallback(void* window, double _, double yoffset)
{
	GLFWwindow* glfwWin = reinterpret_cast<GLFWwindow*>(window);
	Window* w = reinterpret_cast<Window *>( glfwGetWindowUserPointer(glfwWin));
	
	w->mMouseWheel += yoffset;
}

void Window::initialize(int width, int heigth, const std::string& windowTitle)
{
	assert(width > 0 && heigth > 0);
	mWidth = width;
	mHeight = heigth;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mWindow = glfwCreateWindow(width, heigth, windowTitle.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(reinterpret_cast<GLFWwindow*>(mWindow), this);
	glfwSetInputMode(reinterpret_cast<GLFWwindow*>(mWindow),
		GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
	glfwSetScrollCallback(reinterpret_cast<GLFWwindow*>(mWindow), 
		reinterpret_cast<GLFWscrollfun>(s_mouseWheelCallback));
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

void Window::update()
{
#define d_cast(x) static_cast<uint32_t>(Input::x)
	GLFWwindow* w = reinterpret_cast<GLFWwindow*>(mWindow);
	// get mouse inputs
	{
		int state = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_1);
		if (mInputState[d_cast(MouseLeft)].justPressed) {
			mInputState[d_cast(MouseLeft)].justPressed = 0;
			if (state == GLFW_PRESS) {
				mInputState[d_cast(MouseLeft)].pressed = 1;
			}
		}
		else if (mInputState[d_cast(MouseLeft)].pressed) {
			if (state != GLFW_PRESS) {
				mInputState[d_cast(MouseLeft)].pressed = 0;
			}
		}
		else if (state == GLFW_PRESS) {
			mInputState[d_cast(MouseLeft)].justPressed = 1;
		}

		state = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_2);
		if (mInputState[d_cast(MouseRight)].justPressed) {
			mInputState[d_cast(MouseRight)].justPressed = 0;
			if (state == GLFW_PRESS) {
				mInputState[d_cast(MouseRight)].pressed = 1;
			}
		}
		else if (mInputState[d_cast(MouseRight)].pressed) {
			if (state != GLFW_PRESS) {
				mInputState[d_cast(MouseRight)].pressed = 0;
			}
		}
		else if (state == GLFW_PRESS) {
			mInputState[d_cast(MouseLeft)].justPressed = 1;
		}
	}

	// flip mouse wheel buffers
	{
		mMouseWheelBuff = mMouseWheel;
		mMouseWheel = 0;
	}

#undef d_cast
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

bool Window::isJustPressed(Input in) const
{
	return mInputState[static_cast<uint32_t>(in)].justPressed == 1;
}

bool Window::isPressed(Input in) const
{
	return mInputState[static_cast<uint32_t>(in)].pressed == 1;
}

bool Window::isDown(Input in) const
{
	return isJustPressed(in) || isPressed(in);
}

void Window::getMousePosition(std::array<double, 2>* pos) const
{
	glfwGetCursorPos(reinterpret_cast<GLFWwindow*>(mWindow), pos->data(), pos->data() + 1);
}






}; // namespace vkg
}; // namespace gr