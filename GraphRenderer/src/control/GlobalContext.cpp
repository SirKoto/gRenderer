#include "GlobalContext.h"

#include "../utils/serialization.h"
#include <cereal/cereal.hpp>
#include <fstream>

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

void gr::GlobalContext::saveProject() const
{
	std::filesystem::path resourcesPath = mProjectPath;
	resourcesPath /= "Resources.json";
	std::ofstream stream(resourcesPath, std::ofstream::out | std::ofstream::trunc);

	cereal::JSONOutputArchive archive(stream, cereal::JSONOutputArchive::Options::Default());

	archive( GR_SERIALIZE_NVP_MEMBER(mDict));
}

void gr::GlobalContext::destroy()
{
	mWindow.destroy(mRenderContext.getInstance());
	mRenderContext.destroy();
}
