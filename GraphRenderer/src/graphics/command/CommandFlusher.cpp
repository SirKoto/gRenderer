#include "CommandFlusher.h"

namespace gr
{
namespace vkg
{

uint32_t CommandFlusher::createNewBlock(Type type)
{
	uint32_t idx = std::numeric_limits<uint32_t>::max();
	switch (type)
	{
	case gr::vkg::CommandFlusher::Type::eGRAPHICS:
		mGraphicsBlocks.push_back({});
		idx = static_cast<uint32_t>(mGraphicsBlocks.size() - 1);
		break;
	case gr::vkg::CommandFlusher::Type::eTRANSFER:
		mTransferBlocks.push_back({});
		idx = static_cast<uint32_t>(mTransferBlocks.size() - 1);
		break;
	default:
		assert(false);
	}
	return idx;
}
void CommandFlusher::flush(vk::Fence graphicsSignalFence)
{
	
	std::vector<vk::SubmitInfo> submits;
	std::vector<vk::TimelineSemaphoreSubmitInfo> semaphoreInfo;

	submits.reserve(std::max(mTransferBlocks.size(), mGraphicsBlocks.size()));
	semaphoreInfo.reserve(std::max(mTransferBlocks.size(), mGraphicsBlocks.size()));


	for (const FlushBlock& blk : mTransferBlocks) {

		semaphoreInfo.push_back(
		vk::TimelineSemaphoreSubmitInfo(
			blk.waitValues,
			blk.signalValues)
		);

		submits.push_back(
			vk::SubmitInfo(
				blk.waitSemaphores,
				blk.waitStages,
				blk.commands,
				blk.signalSemaphores
			));
		submits.back().setPNext(&semaphoreInfo.back());
	}


	mTransferQueue.submit(submits, nullptr);

	submits.clear();
	semaphoreInfo.clear();

	for (const FlushBlock& blk : mGraphicsBlocks) {

		semaphoreInfo.push_back(
			vk::TimelineSemaphoreSubmitInfo(
				blk.waitValues,
				blk.signalValues)
		);

		submits.push_back(
			vk::SubmitInfo(
				blk.waitSemaphores,
				blk.waitStages,
				blk.commands,
				blk.signalSemaphores
			));
		submits.back().setPNext(&semaphoreInfo.back());
	}

	mGraphicsQueue.submit(submits, graphicsSignalFence);

	for (FlushBlock& blk : mTransferBlocks) {
		blk.clear();
	}
	for (FlushBlock& blk : mGraphicsBlocks) {
		blk.clear();
	}
}

void CommandFlusher::pushCB(Type type, uint32_t block, vk::CommandBuffer cmd)
{
	FlushBlock* pBlk = getBlock(type, block);
	pBlk->commands.push_back(cmd);
}

void CommandFlusher::pushWait(const Type type, const uint32_t block, const vk::Semaphore waitSemaphore, const vk::PipelineStageFlags waitStage, const uint64_t waitValue)
{
	FlushBlock* pBlk = getBlock(type, block);
	pBlk->waitSemaphores.push_back(waitSemaphore);
	pBlk->waitStages.push_back(waitStage);
	pBlk->waitValues.push_back(waitValue);
}

void CommandFlusher::pushSignal(const Type type, const uint32_t block, const vk::Semaphore signalSemaphore, const uint64_t signalValue)
{
	FlushBlock* pBlk = getBlock(type, block);
	pBlk->signalSemaphores.push_back(signalSemaphore);
	pBlk->signalValues.push_back(signalValue);
}


CommandFlusher::FlushBlock* CommandFlusher::getBlock(const Type type, const uint32_t block)
{
	FlushBlock* pBlk = nullptr;

	switch (type)
	{
	case gr::vkg::CommandFlusher::Type::eGRAPHICS:
		assert(block < static_cast<uint32_t>(mGraphicsBlocks.size()));
		pBlk = mGraphicsBlocks.data() + block;
		break;
	case gr::vkg::CommandFlusher::Type::eTRANSFER:
		assert(block < static_cast<uint32_t>(mTransferBlocks.size()));
		pBlk = mTransferBlocks.data() + block;
		break;
	default:
		assert(false);
	}

	return pBlk;
}

} // namespace vkg
} // namespace gr