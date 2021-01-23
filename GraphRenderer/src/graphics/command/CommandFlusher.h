#pragma once

#include "../RenderContext.h"

namespace gr
{
class FrameContext;

namespace vkg
{

class CommandFlusher
{
public:

	void setNumControls(const RenderContext& rc, uint32_t num);

	void flush(const FrameContext& rc, vk::Fence graphicsSignalFence);

	void pushGraphicsCB(vk::CommandBuffer cmd) { graphics.push_back(cmd); }
	void pushTransferCB(vk::CommandBuffer cmd) { transfer.push_back(cmd); }

	void pushExternalGraphicsWait(
		const vk::Semaphore semaphores,
		const vk::PipelineStageFlags waitStages) {
			mExternalGraphicsWaitSemaphores.push_back(semaphores); mExternalGraphicsWaitStage.push_back(waitStages);
	};

	void pushExternalGraphicsSignal(
		const vk::Semaphore semaphore
	) {
		mExternalGraphicsSignalSemaphore.push_back(semaphore);
	}

	void destroy(const RenderContext& rc);

protected:

	uint32_t mNumControls;


	std::vector<vk::CommandBuffer> transfer;
	std::vector<vk::CommandBuffer> graphics;

	std::vector<vk::Semaphore> mSemaphores;
	std::vector<vk::PipelineStageFlags> mExternalGraphicsWaitStage;
	std::vector<vk::Semaphore> mExternalGraphicsWaitSemaphores;
	std::vector<vk::Semaphore> mExternalGraphicsSignalSemaphore;
};


} // namespace vkg
} // namespace gr