#pragma once

#include "IAddon.h"

#include <glm/glm.hpp>

#include "../../graphics/resources/Buffer.h"

namespace gr {
namespace addon {

class Camera : public IAddon
{
public:

	Camera(FrameContext* fc) : IAddon(fc) { createUbos(fc); }

	void drawImGuiInspector(FrameContext* fc, GameObject* parent) override;

	void updateBeforeRender(FrameContext* fc, GameObject* parent) override;

	void destroy(FrameContext* fc) override;

	static const char* s_getAddonName() { return "Camera"; }

protected:

	float mFov = 90.0f;
	float mNear = 0.1f;
	float mFar = 100.0f;
	glm::vec2 mAspectRatio = glm::vec2(16.0f, 9.0f);

	bool mCameraIsControlable = false;

	vkg::Buffer mUbos;
	uint8_t* mUbosGpuPtr = nullptr;
	std::vector<vk::DescriptorSet> mCameraDescriptorSets;

	void createUbos(FrameContext* fc);

};


} // namespace addon
} // namespace gr
