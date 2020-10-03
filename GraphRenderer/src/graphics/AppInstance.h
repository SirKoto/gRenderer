#pragma once

#include <vulkan/vulkan.hpp>

namespace gr
{
namespace vkg
{

	class AppInstance
	{
	public:
		AppInstance(const std::vector<const char*>& extensionsToLoad = {}, bool loadGLFWextensions = true);

		~AppInstance();

		// For the moment, do not allow copy of this
		AppInstance& operator=(const AppInstance&) = delete;

		// Cast to instance
		operator const vk::Instance () const { return mInstance; }


		void destroy();

		const vk::Instance& getInstance() const { return mInstance; }

	private:
		vk::Instance mInstance;
		vk::DebugUtilsMessengerEXT mDebugMessenger;
	};
}; // namespace vkg
}; // namespace gr

