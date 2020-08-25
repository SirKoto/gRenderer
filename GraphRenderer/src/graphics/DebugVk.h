#pragma once
#include <vulkan/vulkan.hpp>

namespace DebugVk
{
#ifdef _DEBUG 
	const bool validationLayersVkEnabled = true;
#else
	const bool validationLayersVkEnabled = false;
#endif // DEBUG

	constexpr const char* debugExtensionName  =
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	constexpr std::array<const char*, 1> validationLayersArray = 
		{ "VK_LAYER_KHRONOS_validation" };

	vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo();

	bool checkValidationLayersSupport();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	vk::Result CreateDebugUtilsMessengerExt(vk::Instance instance,
		const vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const vk::AllocationCallbacks* pAllocator,
		vk::DebugUtilsMessengerEXT* pDebugMessenger);

	void DestroyDebugUtilsMessengerEXT(vk::Instance instance,
		vk::DebugUtilsMessengerEXT debugMessenger,
		const vk::AllocationCallbacks* pAllocator);

}; // Namespace DebugVK

