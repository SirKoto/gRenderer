#include "AppInstance.h"

#include "DebugVk.h"
#include "Window.h"


namespace gr
{
namespace vkg
{

AppInstance::AppInstance(const std::vector<const char*>& extensionsToLoad, 
	bool loadGLFWextensions)
{
	if (DebugVk::validationLayersVkEnabled) {
		bool validationLayersExist = DebugVk::checkValidationLayersSupport();
		if (!validationLayersExist) {
			throw std::runtime_error("Error: no validation layers support!!!");
		}
	}

	// append and put all extensions needed into vector
	std::vector<const char*> extensions(extensionsToLoad);
	if (loadGLFWextensions) {
		const std::vector<const char*> glfwExt = Window::getWindowVkExtensions();
		extensions.insert(extensions.end(), glfwExt.begin(), glfwExt.end());
	}

	// Fill appliaction information
	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "GraphRenderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "GraphRendererEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 2, 154);

	vk::InstanceCreateInfo createInfo = {};
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo;
	if (DebugVk::validationLayersVkEnabled) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(DebugVk::validationLayersArray.size());
		createInfo.ppEnabledLayerNames = DebugVk::validationLayersArray.data();

		debugMessengerInfo = DebugVk::createDebugMessengerCreateInfo();
		createInfo.pNext = &debugMessengerInfo;
	}

	// Create Vulkan Instance
	mInstance = vk::createInstance(createInfo, nullptr);

	// Create debug messenger
	if (DebugVk::validationLayersVkEnabled) {
		vk::Result res = DebugVk::CreateDebugUtilsMessengerExt(mInstance,
			&debugMessengerInfo,
			nullptr,
			&mDebugMessenger);
		if (res != vk::Result::eSuccess) {
			throw std::runtime_error("Cannot dispatch function  to setup debug messenger!!");
		}
	}
}

AppInstance::~AppInstance()
{
}

void AppInstance::destroy()
{
	if (DebugVk::validationLayersVkEnabled) {
		DebugVk::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
	}
	mInstance.destroy();
}

}; // namespace vkg
}; // namespace gr