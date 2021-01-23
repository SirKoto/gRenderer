#include "CommandFlusher.h"

#include "../../control/FrameContext.h"

namespace gr
{
namespace vkg
{

void CommandFlusher::setNumControls(
	const RenderContext& rc, 
	uint32_t num)
{
	mNumControls = num;
	uint32_t actualSize = static_cast<uint32_t>(mSemaphores.size());
	if (num > actualSize) {
		mSemaphores.resize(num);
		for (uint32_t i = actualSize; i < num; ++i) {
			mSemaphores[i] = rc.createSemaphore();
		}
	}
	else {
		// not implemented yet
		assert(false);
	}
}

void CommandFlusher::flush(const FrameContext& fc, vk::Fence graphicsSignalFence)
{
	vk::SubmitInfo info(
		0, nullptr, nullptr,
		static_cast<uint32_t>(transfer.size()),
		transfer.data(),
		1, mSemaphores.data() + fc.getImageIdx()
	);

	fc.rc().getTransferQueue().submit({ info }, nullptr);
	
	mExternalGraphicsWaitSemaphores.push_back(mSemaphores[fc.getImageIdx()]);
	mExternalGraphicsWaitStage.push_back(vk::PipelineStageFlagBits::eTopOfPipe);

	info = vk::SubmitInfo  (
		static_cast<uint32_t>(mExternalGraphicsWaitSemaphores.size()),
		mExternalGraphicsWaitSemaphores.data(), mExternalGraphicsWaitStage.data(),
		static_cast<uint32_t>(graphics.size()),
		graphics.data(),
		static_cast<uint32_t>(mExternalGraphicsSignalSemaphore.size()),
		mExternalGraphicsSignalSemaphore.data()
	);

	fc.rc().getGraphicsQueue().submit({ info }, graphicsSignalFence);


	graphics.clear();
	transfer.clear();
	mExternalGraphicsWaitSemaphores.clear();
	mExternalGraphicsWaitStage.clear();
	mExternalGraphicsSignalSemaphore.clear();
}

void CommandFlusher::destroy(const RenderContext& rc)
{
	for (vk::Semaphore s : mSemaphores) {
		rc.destroy(s);
	}
	mSemaphores.clear();
}

} // namespace vkg
} // namespace gr