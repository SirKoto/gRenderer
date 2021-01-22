#include "FrameContext.h"


void gr::FrameContext::updateTime(double_t newTime)
{
	double_t dt = newTime - mTime;
	mTime = newTime;

	mDeltaTime = 0.0f;
	for (uint32_t i = 0; i < static_cast<uint32_t>(mDeltaTimes.size()) - 1; ++i) {
		mDeltaTime += mDeltaTimes[i + 1];
		mDeltaTimes[i] = mDeltaTimes[i + 1];
	}
	mDeltaTimes.back() = dt;
	mDeltaTime = (mDeltaTime + dt) / mDeltaTimes.size();
}

void gr::FrameContext::resetFrameResources()
{
	graphicsPool().reset();
	presentPool().reset();
	transferPool().reset();
}

void gr::FrameContext::recreateCommandPools()
{
	destroyCommandPools();
	mPools = rc().createCommandPools();
}

void gr::FrameContext::destroyCommandPools()
{
	if (mPools.graphicsPool.get()) {
		rc().destroyCommandPools(&mPools);
	}
}
