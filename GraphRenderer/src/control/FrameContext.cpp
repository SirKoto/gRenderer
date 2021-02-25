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
	if (buffer) {
		mResourcesToDelete.push_back(std::unique_ptr<DelRes>(new DelResBuffer(buffer)));
	}
}

void gr::FrameContext::scheduleToDelete(const vkg::Image2D image)
{
	if (image) {
		mResourcesToDelete.push_back(std::unique_ptr<DelRes>(new DelResImage(image)));
	}
}

void gr::FrameContext::scheduleToDelete(const vk::Sampler sampler)
{
	if (sampler) {
		mResourcesToDelete.push_back(std::unique_ptr<DelRes>(new DelResSampler(sampler)));
	}
}

void gr::FrameContext::resetFrameResources()
{
	graphicsPool().reset();
	presentPool().reset();
	transferPool().reset();

	for (size_t i = 0; i < mResourcesToDelete.size(); ++i) {
		(mResourcesToDelete[i])->destroy(mGlobalContext);
	}

	mResourcesToDelete.clear();
}

void gr::FrameContext::recreateCommandPools()
{
	destroyCommandPools();
	rc().createCommandPools(&mPools);
}

void gr::FrameContext::destroy()
{
	resetFrameResources();
	destroyCommandPools();
}

void gr::FrameContext::destroyCommandPools()
{
	if (mPools.graphicsPool.get()) {
		rc().destroyCommandPools(&mPools);
	}
}

gr::FrameContext::DelResBuffer::DelResBuffer(const vkg::Buffer& buffer)
{
	std::memcpy(&mBuffer, &buffer, sizeof(buffer));
}

void gr::FrameContext::DelResBuffer::destroy(GlobalContext* gc)
{
	gc->rc().destroy(reinterpret_cast<vkg::Buffer&>(mBuffer));
}

gr::FrameContext::DelResImage::DelResImage(const vkg::Image2D& image)
{
	std::memcpy(&mBuffer, &image, sizeof(image));
}

void gr::FrameContext::DelResImage::destroy(GlobalContext* gc)
{
	gc->rc().destroy(reinterpret_cast<vkg::Image2D&>(mBuffer));
}

gr::FrameContext::DelResSampler::DelResSampler(const vk::Sampler& sampler)
{
	std::memcpy(&mBuffer, &sampler, sizeof(sampler));
}

void gr::FrameContext::DelResSampler::destroy(GlobalContext* gc)
{
	gc->rc().destroy(reinterpret_cast<vk::Sampler&>(mBuffer));
}
