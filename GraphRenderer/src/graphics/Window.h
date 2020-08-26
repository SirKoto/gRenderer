#pragma once
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>

class Window
{
public:
	Window() = delete;
	Window(int width, int heigth, const std::string& windowTitle);

	int getWidth() const { return mWidth; }
	int getHeigth() const {	return mHeight; }

	void createVkSurface(
		const vk::Instance& instance
	);

	bool windowShouldClose() const;

	const vk::SurfaceKHR& getSurface() const { return mSurface; }

	void destroy(const vk::Instance& instance);

	static void pollEvents();

	static std::vector<const char*> getWindowVkExtensions();

private:

	const int mWidth, mHeight;
	void* mWindow;
	vk::SurfaceKHR mSurface;

};

