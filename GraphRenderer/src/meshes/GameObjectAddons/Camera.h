#pragma once

#include "IAddon.h"

#include <glm/glm.hpp>

#include "../../graphics/resources/Buffer.h"

namespace gr {
namespace addon {

class Camera : public IAddon
{
public:

	Camera() = default;

	void drawImGuiInspector(FrameContext* fc, GameObject* parent) override;

	std::unique_ptr<addon::IAddon> duplicate(FrameContext* fc, const GameObject* parent) const override final;

	void updateBeforeRender(FrameContext* fc, GameObject* parent, const SceneRenderContext& src) override;

	void start(FrameContext* fc) override { createUbos(fc); };
	void destroy(FrameContext* fc) override;

	const char* getAddonName() override { return Camera::s_getAddonName(); }


	static const char* s_getAddonName() { return "Camera"; }

protected:

	float mFov = 90.0f;
	float mNear = 0.1f;
	float mFar = 100.0f;
	glm::vec2 mAspectRatio = glm::vec2(16.0f, 9.0f);

	vkg::Buffer mUbos;
	uint8_t* mUbosGpuPtr = nullptr;
	std::vector<vk::DescriptorSet> mCameraDescriptorSets;

	void createUbos(FrameContext* fc);

	// Serialization functions
	template<class Archive>
	void serialize(Archive& ar)
	{
		ar(GR_SERIALIZE_NVP_MEMBER(mFov),
			GR_SERIALIZE_NVP_MEMBER(mNear),
			GR_SERIALIZE_NVP_MEMBER(mFar),
			GR_SERIALIZE_NVP_MEMBER(mAspectRatio)
			);
	}

	GR_SERIALIZE_PRIVATE_MEMBERS

};


} // namespace addon
} // namespace gr

GR_SERIALIZE_TYPE(gr::addon::Camera)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::addon::IAddon, gr::addon::Camera)