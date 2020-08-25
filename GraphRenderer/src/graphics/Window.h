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
		const vk::Instance& instance,
		vk::SurfaceKHR* surface
	) const;

	bool windowShouldClose() const;

	static void pollEvents();

	static std::vector<const char*> getWindowVkExtensions();

private:

	const int mWidth, mHeight;
	void* mWindow;

};

