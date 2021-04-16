#pragma once

#include <vulkan/vulkan.hpp>

namespace gr
{

namespace vkg
{

class CommandFlusher
{
public:
	CommandFlusher() = default;
	CommandFlusher(vk::Queue graphicsQueue, vk::Queue transferQueue) : mGraphicsQueue(graphicsQueue), mTransferQueue(transferQueue) {}

	enum class Type {
		eGRAPHICS,
		eTRANSFER
	};

	uint32_t createNewBlock(Type type);

	void flush(vk::Fence graphicsSignalFence = nullptr);

	void pushGraphicsCB(const uint32_t block, const vk::CommandBuffer cmd) { pushCB(Type::eGRAPHICS, block, cmd); }
	void pushTransferCB(const uint32_t block, const vk::CommandBuffer cmd) { pushCB(Type::eTRANSFER, block, cmd); }

	void pushCB(const Type type, const uint32_t block, const vk::CommandBuffer cmd);
	void pushWait(
		const Type type, const uint32_t block, const vk::Semaphore waitSemaphore,
		const vk::PipelineStageFlags waitStage, const uint64_t waitValue = 0);
	void pushSignal(
		const Type type, const uint32_t block, const vk::Semaphore signalSemaphore, const uint64_t signalValue = 0);


protected:

	struct FlushBlock {
		std::vector<vk::CommandBuffer> commands;
		std::vector<vk::Semaphore> waitSemaphores;
		std::vector<vk::PipelineStageFlags> waitStages;
		std::vector<uint64_t> waitValues;
		std::vector<vk::Semaphore> signalSemaphores;
		std::vector<uint64_t> signalValues;

		void clear() {
			this->commands.clear();
			this->signalSemaphores.clear();
			this->signalValues.clear();
			this->waitSemaphores.clear();
			this->waitStages.clear();
			this->waitValues.clear();
		}
	};

	std::vector<FlushBlock> mTransferBlocks;
	std::vector<FlushBlock> mGraphicsBlocks;

	vk::Queue mTransferQueue;
	vk::Queue mGraphicsQueue;


	FlushBlock* getBlock(const Type type, const uint32_t block);

};


} // namespace vkg
} // namespace gr