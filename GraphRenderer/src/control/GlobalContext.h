#pragma once

#include "../graphics/RenderContext.h"
#include "../graphics/Window.h"
#include "../meshes/ResourceDictionary.h"

#include <filesystem>

namespace gr
{

class FrameContext;

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

	ResId getBoundScene() const { return mBoundScene; }
	void setBoundScene(ResId id) { mBoundScene = id; }

	std::filesystem::path getAbsolutePathTo(const std::filesystem::path& relative) const;
	const std::filesystem::path& getProjectPath() const { return mProjectPath; }
	std::filesystem::path& getProjectPath() { return mProjectPath; }
	void setProjectPath(const std::filesystem::path& newPath) { mProjectPath = newPath; }

	void saveProject() const;
	bool loadProject(FrameContext *fc, const std::filesystem::path& projectPath);

	void destroy();

private:
	double_t mGlobalTime = 0.0;
	std::array<double_t, 3> mDeltaTimes;

	vkg::RenderContext mRenderContext;
	vkg::Window mWindow;
	ResourceDictionary mDict;

	ResId mBoundScene;

	std::filesystem::path mProjectPath = {};

};

} // namespace gr
