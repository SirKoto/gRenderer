#include "FrameContext.h"


std::vector<gr::FrameContext> gr::FrameContext::createContexts(
	uint32_t num, GlobalContext* globalContext)
{
	assert(globalContext);
	if (!globalContext->rc().getDevice()) {
		throw std::logic_error("Device not created in contexts creation!!");
	}


	std::vector<FrameContext> contexts;
	contexts.reserve(num);
	for (uint32_t i = 0; i < num; ++i) {
		contexts.emplace_back(FrameContext{ num, i, globalContext });
		contexts.back().recreateCommandPools();
	}

	return contexts;
}



void gr::FrameContext::updateTime(double_t newTime)
{
	mGlobalContext->setTime(newTime);
	mTime = newTime;

	
	mDeltaTime = mGlobalContext->computeDeltaTime();
}

void gr::FrameContext::scheduleToDelete(const vkg::Buffer buffer)
{
	mBuffersToDelete.push_back(buffer);
}

void gr::FrameContext::resetFrameResources()
{
	graphicsPool().reset();
	presentPool().reset();
	transferPool().reset();

	for (const vkg::Buffer buffer : mBuffersToDelete) {
		rc().destroy(buffer);
	}

	mBuffersToDelete.clear();
}

void gr::FrameContext::recreateCommandPools()
{
	destroyCommandPools();
	rc().createCommandPools(&mPools);
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
