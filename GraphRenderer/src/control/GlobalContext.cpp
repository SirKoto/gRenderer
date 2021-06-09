#include "GlobalContext.h"

#include "../utils/serialization.h"
#include "FrameContext.h"

#include <cereal/cereal.hpp>
#include <fstream>

constexpr const char* RESOURCES_FILE = "Resources.json";

void gr::GlobalContext::setTime(double_t newTime)
{
	double_t dt = newTime - this->getTime();
	this->mGlobalTime = newTime;

	for (uint32_t i = 0; i < static_cast<uint32_t>(this->mDeltaTimes.size()) - 1; ++i) {
		this->mDeltaTimes[i] = this->mDeltaTimes[i + 1];
	}
	this->mDeltaTimes.back() = dt;
}

double_t gr::GlobalContext::computeDeltaTime() const
{
    double_t dt = 0.0;
	for (uint32_t i = 0; i < static_cast<uint32_t>(this->mDeltaTimes.size()); ++i) {
		dt += this->mDeltaTimes[i] / this->mDeltaTimes.size();
	}
    return dt;
}

std::filesystem::path gr::GlobalContext::getAbsolutePathTo(const std::filesystem::path& relative) const
{
	if (relative.is_absolute()) {
		return relative;
	}
	return getProjectPath() / relative;
}

void gr::GlobalContext::saveProject() const
{
	std::filesystem::path resourcesPath = mProjectPath;
	resourcesPath /= RESOURCES_FILE;
	std::ofstream stream(resourcesPath, std::ofstream::out | std::ofstream::trunc);

	cereal::JSONOutputArchive archive(stream, cereal::JSONOutputArchive::Options::Default());

	archive(GR_SERIALIZE_NVP_MEMBER(mBoundScene));
	archive( GR_SERIALIZE_NVP_MEMBER(mDict));
}

bool gr::GlobalContext::loadProject(FrameContext* fc, const std::filesystem::path& newPath)
{
	std::filesystem::path resourcesPath = newPath;
	resourcesPath /= RESOURCES_FILE;
	std::ifstream stream(resourcesPath, std::ofstream::in);

	if (!stream) {
		return false;
	}

	cereal::JSONInputArchive archive(stream);

	archive(mBoundScene);

	if (!mDict.empty()) {
		mDict.clear(fc);
	}

	archive( mDict );

	this->setProjectPath(newPath);

	mDict.startAll(fc);

	return true;
}

void gr::GlobalContext::addNewLog(const std::string& log) const
{
	if (mLogFun) {
		mLogFun(log);
	}
}

void gr::GlobalContext::setLogCallback(const std::function<void(const std::string&)>& callback)
{
	mLogFun = callback;
}

void gr::GlobalContext::destroy()
{
	mWindow.destroy(mRenderContext.getInstance());
	mRenderContext.destroy();
}
