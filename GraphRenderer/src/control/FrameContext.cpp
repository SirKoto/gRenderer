#include "FrameContext.h"


std::vector<gr::FrameContext> gr::FrameContext::createContexts(uint32_t num, gr::vkg::RenderContext* rc)
{
	assert(rc);
	if (!rc->getDevice()) {
		throw std::logic_error("Device not created in contexts creation!!");
	}

	std::shared_ptr<TimeHandler> timeHandler = std::make_shared<TimeHandler>();

	std::vector<FrameContext> contexts;
	contexts.reserve(num);
	for (uint32_t i = 0; i < num; ++i) {
		contexts.push_back(FrameContext{ num, i, rc, timeHandler });
		contexts.back().recreateCommandPools();
	}

	return contexts;
}

void gr::FrameContext::updateTime(double_t newTime)
{
	double_t dt = newTime - mTimeHandler->globalTime;
	mTimeHandler->globalTime = newTime;
	mTime = newTime;

	mDeltaTime = 0.0f;
	for (uint32_t i = 0; i < static_cast<uint32_t>(mTimeHandler->deltaTimes.size()) - 1; ++i) {
		mDeltaTime += mTimeHandler->deltaTimes[i + 1];
		mTimeHandler->deltaTimes[i] = mTimeHandler->deltaTimes[i + 1];
	}
	mTimeHandler->deltaTimes.back() = dt;
	mDeltaTime = (mDeltaTime + dt) / mTimeHandler->deltaTimes.size();
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

void gr::FrameContext::destroy()
{
	destroyCommandPools();
}

void gr::FrameContext::destroyCommandPools()
{
	if (mPools.graphicsPool.get()) {
		rc().destroyCommandPools(&mPools);
	}
}
