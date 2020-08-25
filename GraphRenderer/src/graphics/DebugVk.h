#pragma once
#include <vulkan/vulkan.hpp>

namespace DebugVk
{
#ifdef _DEBUG 
	const bool validationLayersVkEnabled = true;
#else
	const bool validationLayersVkEnabled = false;
#endif // DEBUG

	constexpr const char* getVkDebugExtensionName() {
		return VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	}

	static const std::vector<const char*> getValidationLayers() {
		return { "VK_LAYER_KHRONOS_validation" };
	}

	static vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo();

	static bool checkValidationLayersSupport();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

};

