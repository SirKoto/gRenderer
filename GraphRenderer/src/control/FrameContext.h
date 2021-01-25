#pragma once
#include "../graphics/RenderContext.h"

namespace gr
{

class FrameContext
{
public:

	static std::vector<FrameContext> createContexts(uint32_t num, vkg::RenderContext* rc);

	FrameContext() = default;

	FrameContext(const FrameContext& o) = default;

	vkg::RenderContext& rc() { return *mRenderContext; }
	const vkg::RenderContext& rc() const { return *mRenderContext; }

	// From 0 to MAX_FRAMES_IN_FLIGHT
	uint32_t getIdx() const { return mFrameId; }
	uint32_t getNumConcurrentFrames() const { return CONCURRENT_FRAMES; }
	void advanceFrameCount() { mFrameCount += CONCURRENT_FRAMES; }
	uint64_t getFrameCount() const { return mFrameCount; }
	uint64_t getNextFrameCount() const { return mFrameCount + CONCURRENT_FRAMES; }
	void setImageIdx(uint32_t idx) { mImageIdx = idx; }
	uint32_t getImageIdx() const { return mImageIdx; }

	void updateTime(double_t newTime);

	double_t time() const { return mTime; }
	double_t dt() const { return mDeltaTime; }
	float_t timef() const { return static_cast<float_t>(mTime); }
	float_t dtf() const { return static_cast<float_t>(mDeltaTime); }

	vkg::ResetCommandPool& graphicsPool() { return mPools.graphicsPool; };
	const vkg::ResetCommandPool& graphicsPool() const { return mPools.graphicsPool; };
	vkg::ResetCommandPool& presentPool() { return mPools.presentPool; };
	const vkg::ResetCommandPool& presentPool() const { return mPools.presentPool; };
	vkg::ResetCommandPool& transferPool() { return mPools.transferTransientPool; };
	const vkg::ResetCommandPool& transferPool() const { return mPools.transferTransientPool; };

	void resetFrameResources();
	void recreateCommandPools();

	void destroy();

private:
	uint32_t CONCURRENT_FRAMES = 0;
	uint32_t mFrameId = 0;
	uint64_t mFrameCount = 0;
	uint32_t mImageIdx = 0;
	double_t mTime = 0.0;
	double_t mDeltaTime = 1/30.0;
	vkg::RenderContext* mRenderContext;

	vkg::RenderContext::CommandPools mPools;

	struct TimeHandler {
		double_t globalTime = 0.0;
		std::array<double_t, 3> deltaTimes;
		TimeHandler() { deltaTimes.fill(1 / 30.0); }
	};

	std::shared_ptr<TimeHandler> mTimeHandler;


	void destroyCommandPools();

	FrameContext(uint32_t numMax, uint32_t id, vkg::RenderContext* rc, const std::shared_ptr<TimeHandler>& timeHandler) :
		CONCURRENT_FRAMES(numMax), mFrameId(id), mFrameCount(id), mRenderContext(rc), mTimeHandler(timeHandler) {}

};

}