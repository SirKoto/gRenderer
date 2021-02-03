#pragma once

#include <set>

#include "../command/FreeCommandPool.h"
#include "../resources/Buffer.h"
#include "../resources/Image2D.h"

namespace gr
{
namespace vkg
{

class RenderContext;
class BufferTransferer
{
public:

	BufferTransferer(const vk::DeviceSize minSize = 1 << 24);

	void setUpTransferBlocks(RenderContext* rc);

	void updateAndFlushTransfers(RenderContext* rc,
		vk::Semaphore* outSemaphore = nullptr,
		uint64_t* outValue = nullptr);

	void transferToBuffer(
		const RenderContext& rc,
		const void* data,
		const vk::DeviceSize numBytes,
		const Buffer& dstBuffer,
		const vk::DeviceSize dstOffset = 0);

	// Transfer image into given dstImage
	// If transferToGraphics == false, then the image is 
	// acquired by the transfer family.
	// dstStageMask can only be Fragment at the moment
	void transferToImage(
		const RenderContext& rc,
		const void* data,
		const vk::DeviceSize numBytes,
		const Image2D& dstImage,
		const vk::ImageSubresourceLayers& layerInfo,
		const vk::AccessFlags dstAccessMask,
		const vk::ImageLayout dstImageLayout,
		const vk::PipelineStageFlags dstStageMask,
		const bool transferToGraphics = true
	);


	// Destroy before destroying command pools
	void destroy(RenderContext* rc);

protected:


	struct StageBuffer {
		Buffer buffer;
		vk::DeviceSize size;
		std::set<vk::DeviceSize, std::greater<vk::DeviceSize>> lastAvailableByte;
		uint8_t* ptr;

		void create(const RenderContext& rc, vk::DeviceSize size);
		void free(vk::DeviceSize addr);

		vk::DeviceSize getLastAvailableByte() const;

		void reserve(vk::DeviceSize bytes);

		void destroy(const RenderContext& rc);
	};

	struct TransferOp {
		uint32_t stageBuffIdx;
		vk::DeviceSize srcOffset;
		vk::Buffer dstBuffer;
		vk::DeviceSize dstOffset;
		vk::DeviceSize bytes;
	};

	struct ImageTransferOp {
		uint32_t stageBuffIdx;
		vk::DeviceSize srcOffset;
		vk::Image dstImage;
		vk::ImageSubresourceLayers layersInfo;
		vk::Extent3D extent;
		vk::AccessFlags dstAccessMask;
		vk::ImageLayout dstImageLayout;

		bool acquireGraphics = true;
	};

	struct TransferSpace
	{
		vk::Semaphore semaphore;
		uint64_t value = 0;
		bool inUse = false;
		FreeCommandPool::FreeCommandBuffer transferCmd;
		FreeCommandPool::FreeCommandBuffer graphicsCmd;
		std::vector<TransferOp> bufferTransferOps;
		std::vector<ImageTransferOp> imageTransfersFragmentOps;

		void reset(RenderContext* rc);

		bool empty() const;

		TransferSpace(vk::Semaphore s) : semaphore(s) {}
	};
	

	

	vk::DeviceSize mMinSize;

	std::vector<StageBuffer> mStagingBuffers;
	std::vector<TransferSpace> mTransferSpaces;

	uint32_t mCurrentSpace = std::numeric_limits<uint32_t>::max();


	uint32_t mGraphicsBlock = std::numeric_limits<uint32_t>::max();
	uint32_t mTransferBlock = std::numeric_limits<uint32_t>::max();

	std::mutex mBufferMutex, mImageMutex;

	uint32_t findOrCreateStageBuffer(
		const RenderContext& rc,
		const vk::DeviceSize numBytes);

	uint32_t findOrCreateTransferSpace(const RenderContext& rc);

	void updateTransferStates(RenderContext* rc);


};

} // namespace vkg
} // namespace gr