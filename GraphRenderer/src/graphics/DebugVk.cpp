#include "DebugVk.h"

#include <iostream>

namespace gr
{
namespace vkg
{

vk::DebugUtilsMessengerCreateInfoEXT DebugVk::createDebugMessengerCreateInfo()
{
	vk::DebugUtilsMessengerCreateInfoEXT createInfo;
	createInfo.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError 
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning 
		//| vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
		;

	createInfo.messageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

	createInfo.pfnUserCallback = DebugVk::debugCallback;

	return createInfo;
}

bool DebugVk::checkValidationLayersSupport()
{
	std::vector<vk::LayerProperties> avaliableLayers = vk::enumerateInstanceLayerProperties();

	for (const char* layerName : validationLayersArray) {
		bool layerFound = false;
		for (const VkLayerProperties& layerProperties : avaliableLayers) {
			if (std::strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			return false;
		}
	}
	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugVk::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::string type = [&messageSeverity]() {
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return "VERBOSE";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return "INFO";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return "WARNING";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			return "ERROR";
		default:
			return "TYPE NOT FOUND";
		}}();

	std::cerr << "[" + type + "] validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

vk::Result DebugVk::CreateDebugUtilsMessengerExt(vk::Instance instance,
	const vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const vk::AllocationCallbacks* pAllocator,
	vk::DebugUtilsMessengerEXT* pDebugMessenger){
	auto f = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
		"vkCreateDebugUtilsMessengerEXT");
	if (f != nullptr) {
		return static_cast<vk::Result>( f(instance,
			reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(pCreateInfo), 
			reinterpret_cast<const VkAllocationCallbacks*> (pAllocator), 
			reinterpret_cast<VkDebugUtilsMessengerEXT*> (pDebugMessenger)) );
	}
	else {
		return vk::Result::eErrorExtensionNotPresent;
	}
}

void DebugVk::DestroyDebugUtilsMessengerEXT(vk::Instance instance, vk::DebugUtilsMessengerEXT debugMessenger, const vk::AllocationCallbacks* pAllocator)
{
	auto f = (PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (f != nullptr) {
		f(instance, debugMessenger, reinterpret_cast<const VkAllocationCallbacks*>(pAllocator));
	}
}


}; // namespace vkg
}; // namespace gr