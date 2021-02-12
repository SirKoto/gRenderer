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

void Window::s_charCallback(void* window, unsigned int c)
{
	GLFWwindow* glfwWin = reinterpret_cast<GLFWwindow*>(window);
	Window* w = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWin));
	w->mCharInput.push_back(c);
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
	glfwSetInputMode(reinterpret_cast<GLFWwindow*>(mWindow),
		GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetScrollCallback(reinterpret_cast<GLFWwindow*>(mWindow), 
		reinterpret_cast<GLFWscrollfun>(s_mouseWheelCallback));
	glfwSetCharCallback(reinterpret_cast<GLFWwindow*>(mWindow),
		reinterpret_cast<GLFWcharfun>(s_charCallback));
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
	auto updateState = [&](uint32_t id, int state) {
		if (mInputState[id].justPressed) {
			mInputState[id].justPressed = 0;
			if (state == GLFW_PRESS) {
				mInputState[id].pressed = 1;
			}
		}
		else if (mInputState[id].pressed) {
			if (state != GLFW_PRESS) {
				mInputState[id].pressed = 0;
			}
		}
		else if (state == GLFW_PRESS) {
			mInputState[id].justPressed = 1;
		}
	};
	// get mouse and key inputs
	{
		int state = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_1);
		updateState(d_cast(MouseLeft), state);

		state = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_2);
		updateState(d_cast(MouseRight), state);

		for (uint32_t k = static_cast<uint32_t>(Input::KeyA), i = 0;
			k <= static_cast<uint32_t>(Input::KeyZ);
			++k) {
			state = glfwGetKey(w, GLFW_KEY_A + i++);
			updateState(k, state);
		}

		state = glfwGetKey(w, GLFW_KEY_LEFT);
		updateState(d_cast(ArrowLeft), state);
		state = glfwGetKey(w, GLFW_KEY_DOWN);
		updateState(d_cast(ArrowDown), state);
		state = glfwGetKey(w, GLFW_KEY_RIGHT);
		updateState(d_cast(ArrowRight), state);
		state = glfwGetKey(w, GLFW_KEY_UP);
		updateState(d_cast(ArrowUp), state);


		state = glfwGetKey(w, GLFW_KEY_BACKSPACE);
		updateState(d_cast(KeyBackspace), state);
		state = glfwGetKey(w, GLFW_KEY_DELETE);
		updateState(d_cast(KeyDelete), state);
		state = glfwGetKey(w, GLFW_KEY_TAB);
		updateState(d_cast(KeyTab), state);
		state = glfwGetKey(w, GLFW_KEY_HOME);
		updateState(d_cast(KeyHome), state);
		state = glfwGetKey(w, GLFW_KEY_END);
		updateState(d_cast(KeyEnd), state);
		state = glfwGetKey(w, GLFW_KEY_SPACE);
		updateState(d_cast(KeySpace), state);
		state = glfwGetKey(w, GLFW_KEY_ENTER);
		updateState(d_cast(KeyEnter), state);
		state = glfwGetKey(w, GLFW_KEY_KP_ENTER);
		updateState(d_cast(KeyEnterKeyPad), state);
		state = glfwGetKey(w, GLFW_KEY_ESCAPE);
		updateState(d_cast(KeyEscape), state);
		state = glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) | glfwGetKey(w, GLFW_KEY_RIGHT_CONTROL);
		updateState(d_cast(KeyCtrl), state);
		state = glfwGetKey(w, GLFW_KEY_LEFT_ALT) | glfwGetKey(w, GLFW_KEY_RIGHT_ALT);
		updateState(d_cast(KeyAlt), state);
		state = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) | glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT);
		updateState(d_cast(KeyShift), state);

	}

	// flip mouse wheel buffers
	{
		mMouseWheelBuff = mMouseWheel;
		mMouseWheel = 0;
	}

	// flip char input buffers
	{
		mCharInput.swap(mCharInputBuff);
		mCharInput.clear();
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