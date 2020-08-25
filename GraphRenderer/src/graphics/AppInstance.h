#pragma once

#include <vulkan/vulkan.hpp>

class AppInstance
{
public:
	AppInstance(const std::vector<const char*>& extensionsToLoad = {}, bool loadGLFWextensions = true);

	~AppInstance();

	// For the moment, do not allow copy of this
	AppInstance& operator=(const AppInstance&) = delete;

private:
	vk::Instance mInstance;
	vk::DebugUtilsMessengerEXT mDebugMessenger;
};

