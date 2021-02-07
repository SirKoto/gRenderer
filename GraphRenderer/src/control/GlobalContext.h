#pragma once

#include "../graphics/RenderContext.h"

namespace gr
{


class GlobalContext
{
public:
	GlobalContext() { mDeltaTimes.fill(1 / 30.0); }

	vkg::RenderContext& rc() { return mRenderContext; }
	const vkg::RenderContext& rc() const { return mRenderContext; }

	double_t getTime() const { return mGlobalTime; }
	void setTime(double_t newTime);
	double_t computeDeltaTime() const;

private:
	double_t mGlobalTime = 0.0;
	std::array<double_t, 3> mDeltaTimes;

	vkg::RenderContext mRenderContext;
};

} // namespace gr
