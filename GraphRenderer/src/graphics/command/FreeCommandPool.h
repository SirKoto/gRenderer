#pragma once

#include <vulkan/vulkan.hpp>
#include <mutex>

namespace gr
{
namespace vkg
{


class FreeCommandPool
{
public:
	FreeCommandPool();
	FreeCommandPool(uint32_t familyIdx, vk::CommandPoolCreateFlags flags, vk::Device device);

	FreeCommandPool& operator=(const FreeCommandPool& o);

	struct FreeCommandBuffer {
		vk::CommandBuffer buffer = nullptr;
		uint32_t id = 0;

		FreeCommandBuffer& operator=(std::nullptr_t) 
			{ buffer = nullptr; id = 0; return *this; }

		explicit operator bool() const { return static_cast<bool>(buffer); }

		operator vk::CommandBuffer() { return buffer; }
	};

	// num must be different from zero
	std::vector<FreeCommandBuffer> newCommandBuffers(const uint32_t num);

	FreeCommandBuffer newCommandBuffer();

	void freeCommandBuffers(const std::vector<FreeCommandBuffer>& commands);
	void freeCommandBuffer(const FreeCommandBuffer& command);

	void flushFrees();

	void destroy();

	explicit operator vk::CommandPool() const;

	vk::CommandPool get() const;

protected:

	struct CommandPoolSpace {
		vk::CommandPool pool;
		std::vector<vk::CommandBuffer> toFree;
	};

	std::mutex mFreeMutex;

	std::vector<CommandPoolSpace> mCommandSpace;
	vk::Device mDevice;
	vk::Queue mQueue;

};

} // namespace vkg
} // namespace gr
