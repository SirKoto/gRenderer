#include "BufferTransferer.h"

#include "../RenderContext.h"

namespace gr
{
namespace vkg
{
BufferTransferer::BufferTransferer(const vk::DeviceSize minSize) :
	mMinSize(minSize) 
{
}

void BufferTransferer::setUpTransferBlocks(RenderContext* rc)
{
	mGraphicsBlock = rc->getCommandFlusher()->createNewBlock(CommandFlusher::Type::eGRAPHICS);
	mTransferBlock = rc->getCommandFlusher()->createNewBlock(CommandFlusher::Type::eTRANSFER);
	mCurrentSpace = findOrCreateTransferSpace(*rc);
}

void BufferTransferer::updateAndFlushTransfers(
	RenderContext* rc,
	vk::Semaphore* outSemaphore,
	uint64_t* outValue)
{
	assert((outSemaphore == nullptr) == (outValue == nullptr));

	TransferSpace& ts = mTransferSpaces[mCurrentSpace];

	if (!ts.bufferTransferOps.empty() ||
		!ts.imageTransfersFragmentOps.empty()) {
		ts.value += 1;

		ts.transferCmd = rc->getTransferFreeCommandPool()->newCommandBuffer();
		vk::CommandBuffer transferCmd = ts.transferCmd;
		transferCmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

		// buffer transferences
		vk::BufferCopy cpyInfo;
		for (const TransferOp& op : ts.bufferTransferOps) {
			cpyInfo.setSrcOffset(op.srcOffset)
				.setDstOffset(op.dstOffset)
				.setSize(op.bytes);

			transferCmd.copyBuffer(mStagingBuffers[op.stageBuffIdx].buffer.getVkBuffer(),
				op.dstBuffer, 1, &cpyInfo);
		}

		// image transferences
		// First transition image to dstOptimal
		std::vector<vk::ImageMemoryBarrier> barriers(
			ts.imageTransfersFragmentOps.size(),
			vk::ImageMemoryBarrier(
				vk::AccessFlags{}, // src AccessMask
				vk::AccessFlagBits::eTransferWrite, // dst AccessMask
				vk::ImageLayout::eUndefined,// old layout
				vk::ImageLayout::eTransferDstOptimal,// new layout
				VK_QUEUE_FAMILY_IGNORED,	// src queue family
				VK_QUEUE_FAMILY_IGNORED,	// dst queue family
				nullptr,					// image
				{}							// range
			));
		const uint32_t numImgTransfers = static_cast<uint32_t>(ts.imageTransfersFragmentOps.size());
		for (uint32_t i = 0; i < numImgTransfers; ++i) {		
			const ImageTransferOp& op = ts.imageTransfersFragmentOps[i];
			barriers[i].setImage(op.dstImage)
				.setSubresourceRange(vk::ImageSubresourceRange(
					op.layersInfo.aspectMask,
					op.layersInfo.mipLevel,
					VK_REMAINING_MIP_LEVELS,
					op.layersInfo.baseArrayLayer,
					op.layersInfo.layerCount
				));
		}

		transferCmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe, // src stage mask
			vk::PipelineStageFlagBits::eTransfer, // dst stage mask
			vk::DependencyFlagBits{},
			0, nullptr, 0, nullptr, // buffer and memory barrier
			static_cast<uint32_t>(barriers.size()), barriers.data() // image memory barrier
		);
		// copy image data
		for (uint32_t i = 0; i < numImgTransfers; ++i) {
			const ImageTransferOp& op = ts.imageTransfersFragmentOps[i];

			vk::BufferImageCopy regionInfo(
				op.srcOffset, 0u, 0u,	// offset, row length, image height
				op.layersInfo,		// subresource layers
				vk::Offset3D(0),
				op.extent
			);
			transferCmd.copyBufferToImage(
				mStagingBuffers[op.stageBuffIdx].buffer.getVkBuffer(), // src buffer
				op.dstImage,	// dst image
				vk::ImageLayout::eTransferDstOptimal,			// dst image layout
				1, &regionInfo									// regions
			);
		}

		// Second barriers to set final layout
		uint32_t numGraphicsAcquire = 0;
		for (uint32_t i = 0; i < numImgTransfers; ++i) {
			const ImageTransferOp& op = ts.imageTransfersFragmentOps[i];
			barriers[i]
				.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
				.setDstAccessMask(op.dstAccessMask)
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(op.dstImageLayout);

			if (op.acquireGraphics) {
				numGraphicsAcquire += 1;
				barriers[i]
					.setSrcQueueFamilyIndex(rc->getTransferFamilyIdx())
					.setDstQueueFamilyIndex(rc->getGraphicsFamilyIdx());
			}
		}
		transferCmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer, // src stage mask
			vk::PipelineStageFlagBits::eFragmentShader, // dst stage mask
			vk::DependencyFlagBits{},
			0, nullptr, 0, nullptr, // buffer and memory barrier
			static_cast<uint32_t>(barriers.size()), barriers.data() // image memory barrier
		);

		transferCmd.end();

		// Compute also graphics acquisitions if needed
		vk::CommandBuffer graphicsCmd = nullptr;
		uint64_t transferSignalValue = ts.value;
		if (numGraphicsAcquire > 0) {
			ts.value += 1;
			ts.graphicsCmd = rc->getGraphicsFreeCommandPool()->newCommandBuffer();
			graphicsCmd = ts.graphicsCmd;
			graphicsCmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

			for (uint32_t i = 0, k = 0; k < numGraphicsAcquire; ++i) {
				const ImageTransferOp& op = ts.imageTransfersFragmentOps[i];
				if (op.acquireGraphics) {
					barriers[k++] = barriers[i];
				}
			}


			
			graphicsCmd.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer, // src stage mask
				vk::PipelineStageFlagBits::eFragmentShader, // dst stage mask
				vk::DependencyFlagBits{},
				0, nullptr, 0, nullptr, // buffer and memory barrier
				numGraphicsAcquire, barriers.data() // image memory barrier
			);

			graphicsCmd.end();
		}

		rc->getCommandFlusher()->pushTransferCB(mTransferBlock, transferCmd);
		rc->getCommandFlusher()->pushSignal(CommandFlusher::Type::eTRANSFER,
			mTransferBlock, ts.semaphore, transferSignalValue);
		if (numGraphicsAcquire > 0) {
			rc->getCommandFlusher()->pushGraphicsCB(mGraphicsBlock, graphicsCmd);
			rc->getCommandFlusher()->pushSignal(CommandFlusher::Type::eGRAPHICS,
				mGraphicsBlock, ts.semaphore, ts.value);
			rc->getCommandFlusher()->pushWait(CommandFlusher::Type::eGRAPHICS,
				mGraphicsBlock, ts.semaphore,
				vk::PipelineStageFlagBits::eTransfer, transferSignalValue);
		}
	}

	if (outSemaphore != nullptr) {
		*outSemaphore = ts.semaphore;
		*outValue = ts.value;
	}

	this->updateTransferStates(rc);

}



void BufferTransferer::updateTransferStates(RenderContext* rc)
{

	for (TransferSpace& ts : mTransferSpaces) {
		if (ts.inUse) {
			if (ts.empty()) {
				ts.reset(rc);
			}
			else {
				uint64_t value = rc->getDevice().getSemaphoreCounterValue(ts.semaphore);
				bool finished = value >= ts.value;
				if (finished) {
					for (const TransferOp& op : ts.bufferTransferOps) {
						mStagingBuffers[op.stageBuffIdx].free(op.srcOffset);
					}
					for (const ImageTransferOp& op : ts.imageTransfersFragmentOps) {
						mStagingBuffers[op.stageBuffIdx].free(op.srcOffset);
					}
					ts.reset(rc);
				}
			}
		}
	}

	mCurrentSpace = findOrCreateTransferSpace(*rc);

}

void BufferTransferer::transferToBuffer(
	const RenderContext& rc, const void* data,
	const vk::DeviceSize numBytes, const Buffer& dstBuffer,
	const vk::DeviceSize dstOffset)
{
	mBufferMutex.lock();

	const uint32_t buffIdx = findOrCreateStageBuffer(rc, numBytes);
	const vk::DeviceSize memPos = mStagingBuffers[buffIdx].getLastAvailableByte();
	mStagingBuffers[buffIdx].reserve(numBytes);
	
	mTransferSpaces[mCurrentSpace].bufferTransferOps
		.push_back(
		TransferOp{ buffIdx, memPos,
					dstBuffer.getVkBuffer(), dstOffset,
					numBytes });

	mBufferMutex.unlock();

	std::memcpy(mStagingBuffers[buffIdx].ptr + memPos, data, numBytes);


}

void BufferTransferer::transferToImage(const RenderContext& rc,
	const void* data,
	const vk::DeviceSize numBytes,
	const Image2D& dstImage,
	const vk::ImageSubresourceLayers& layerInfo,
	const vk::AccessFlags dstAccessMask,
	const vk::ImageLayout dstImageLayout,
	const vk::PipelineStageFlags dstStageMask,
	const bool transferToGraphics)
{
	mImageMutex.lock();
	const uint32_t buffIdx = findOrCreateStageBuffer(rc, numBytes);
	const vk::DeviceSize memPos = mStagingBuffers[buffIdx].getLastAvailableByte();
	mStagingBuffers[buffIdx].reserve(numBytes);


	ImageTransferOp op;
	op.stageBuffIdx = buffIdx;
	op.srcOffset = memPos;
	op.dstImage = dstImage.getVkImage();
	op.layersInfo = layerInfo;
	op.extent =  vk::Extent3D(dstImage.getExtent(), 1);
	op.dstAccessMask = dstAccessMask;
	op.dstImageLayout = dstImageLayout;
	op.acquireGraphics = transferToGraphics;

	if (dstStageMask == vk::PipelineStageFlagBits::eFragmentShader) {
		mTransferSpaces[mCurrentSpace].imageTransfersFragmentOps.push_back(op);
	}
	else {
		mImageMutex.unlock();
		throw std::logic_error("Error: dst Pipeline Stage not supported!!");
	}
	mImageMutex.unlock();

	std::memcpy(mStagingBuffers[buffIdx].ptr + memPos, data, numBytes);


}

void BufferTransferer::destroy(RenderContext* rc)
{
	for (StageBuffer& buff : mStagingBuffers) {
		buff.destroy(*rc);
	}
	for (TransferSpace& sem : mTransferSpaces) {
		sem.reset(rc);
		rc->destroy(sem.semaphore);
	}
}

uint32_t BufferTransferer::findOrCreateStageBuffer(
	const RenderContext& rc, const vk::DeviceSize numBytes)
{
	const uint32_t num = static_cast<uint32_t>(mStagingBuffers.size());
	for (uint32_t i = 0; i < num; ++i) {
		const StageBuffer& buf = mStagingBuffers[i];
		if (buf.size - buf.getLastAvailableByte()) {
			return i;
		}
	}

	// Else create buffer with enough size
	mStagingBuffers.push_back({});
	mStagingBuffers.back().create(rc, std::max(mMinSize, numBytes));

	return num;
}

uint32_t BufferTransferer::findOrCreateTransferSpace(const RenderContext& rc)
{
	const uint32_t num = static_cast<uint32_t>(mTransferSpaces.size());
	for (uint32_t i = 0; i < num; ++i) {
		if (!mTransferSpaces[i].inUse) {
			mTransferSpaces[i].inUse = true;
			return i;
		}
	}

	mTransferSpaces.push_back(
		TransferSpace(rc.createTimelineSemaphore(0))
	);
	mTransferSpaces.back().inUse = true;

	return num;
}

void BufferTransferer::StageBuffer::create(const RenderContext& rc, vk::DeviceSize size)
{
	this->buffer = rc.createStagingBuffer(size);
	this->size = size;


	rc.mapAllocatable(this->buffer, reinterpret_cast<void**>(&this->ptr));
}

void BufferTransferer::StageBuffer::free(vk::DeviceSize addr)
{
	this->lastAvailableByte.erase(addr);
}

vk::DeviceSize BufferTransferer::StageBuffer::getLastAvailableByte() const
{
	return this->lastAvailableByte.begin() == this->lastAvailableByte.end() ? 
		0 : this->lastAvailableByte.begin()->second;
}

void BufferTransferer::StageBuffer::reserve(vk::DeviceSize bytes)
{
	vk::DeviceSize src = this->getLastAvailableByte();
	lastAvailableByte.emplace(src, src + bytes);
}

void BufferTransferer::StageBuffer::destroy(const RenderContext& rc)
{
	rc.unmapAllocatable(this->buffer);

	rc.destroy(this->buffer);
}

void BufferTransferer::TransferSpace::reset(RenderContext* rc)
{
	this->inUse = false;
	if (this->graphicsCmd) {
		rc->getGraphicsFreeCommandPool()->freeCommandBuffer(this->graphicsCmd);
	}
	if (this->transferCmd) {
		rc->getTransferFreeCommandPool()->freeCommandBuffer(this->transferCmd);
	}
	this->graphicsCmd = nullptr;
	this->transferCmd = nullptr;

	this->bufferTransferOps.clear();
	this->imageTransfersFragmentOps.clear();
}

bool BufferTransferer::TransferSpace::empty() const
{
	return this->bufferTransferOps.empty() && this->imageTransfersFragmentOps.empty();
}

} // namespace vkg
} // namespace gr