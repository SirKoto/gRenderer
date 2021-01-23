#pragma once
#include "../graphics/RenderContext.h"
#include "../graphics/command/CommandFlusher.h"

namespace gr
{

class FrameContext
{
public:
	FrameContext() { mDeltaTimes.fill(1 / 30.0); };
	FrameContext(uint32_t id, vkg::RenderContext* rc) : mFrameId(id), mRenderContext(rc) {
		mDeltaTimes.fill(1 / 30.0); 
	}
	FrameContext(vkg::RenderContext* rc) : mFrameId(0), mRenderContext(rc) { 
		mDeltaTimes.fill(1 / 30.0); 
	}

	FrameContext(const FrameContext& o) = default;

	vkg::RenderContext& rc() { return *mRenderContext; }
	const vkg::RenderContext& rc() const { return *mRenderContext; }

	void setFrameId(uint32_t id) { mFrameId = id; }
	uint32_t getIdx() const { return mFrameId; }
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

	vkg::CommandFlusher& commandFlusher() { return mCommandFlusher; }
	const vkg::CommandFlusher& commandFlusher() const { return mCommandFlusher; }


	void resetFrameResources();
	void recreateCommandPools();

	void destroy();

private:
	uint32_t mFrameId = 0;
	uint32_t mImageIdx = 0;
	double_t mTime = 0.0;
	std::array<double_t, 3> mDeltaTimes = {};
	double_t mDeltaTime = 1/30.0;
	vkg::RenderContext* mRenderContext;

	vkg::CommandFlusher mCommandFlusher;
	vkg::RenderContext::CommandPools mPools;

	void destroyCommandPools();

};

}