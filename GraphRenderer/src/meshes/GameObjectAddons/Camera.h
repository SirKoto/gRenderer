#pragma once

#include "IAddon.h"

#include <glm/glm.hpp>

namespace gr {
namespace addon {

class Camera : public IAddon
{
public:

	void drawImGuiInspector(FrameContext* fc, GameObject* parent) override;

	void preRenderUpdate(FrameContext* fc, GameObject* parent) override;

	static const char* s_getAddonName() { return "Camera"; }

protected:

	float mFov = 90.0f;
	float mNear = 0.1f;
	float mFar = 100.0f;
	glm::vec2 mAspectRatio = glm::vec2(16.0f, 9.0f);

	bool mCameraIsControlable = false;

};


} // namespace addon
} // namespace gr
