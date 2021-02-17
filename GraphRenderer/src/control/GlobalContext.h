#pragma once

#include "../graphics/RenderContext.h"
#include "../graphics/Window.h"
#include "../meshes/ResourceDictionary.h"

namespace gr
{


class GlobalContext
{
public:
	GlobalContext() { mDeltaTimes.fill(1 / 30.0); }

	vkg::RenderContext& rc() { return mRenderContext; }
	const vkg::RenderContext& rc() const { return mRenderContext; }
	vkg::Window& getWindow() { return mWindow; }
	const vkg::Window& getWindow() const { return mWindow; }
	const ResourceDictionary& getDict() const { return mDict; }
	ResourceDictionary& getDict() { return mDict; }

	double_t getTime() const { return mGlobalTime; }
	void setTime(double_t newTime);
	double_t computeDeltaTime() const;

	void destroy();

private:
	double_t mGlobalTime = 0.0;
	std::array<double_t, 3> mDeltaTimes;

	vkg::RenderContext mRenderContext;
	vkg::Window mWindow;
	ResourceDictionary mDict;
};

} // namespace gr
