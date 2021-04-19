#pragma once

#include "IAddon.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace gr {
namespace addon
{


class Transform : public IAddon
{
public:
	Transform() = default;


	void drawImGuiInspector(FrameContext* fc, GameObject* parent) override final;

	const char* getAddonName() override { return Transform::s_getAddonName(); }

	static const char* s_getAddonName() { return "Transform"; }

	const glm::vec3& getPos() const { return mPos; }
	void setPos (const glm::vec3& pos) { mPos = pos; }
	const glm::vec3& getScale() const { return mScale; }
	void setScale(const glm::vec3& scale) { mScale = scale; }
	const glm::quat& getRotation() const { return mRotation; }
	void setRotation(const glm::quat& rotation) { mRotation = rotation; }
	// Rotate angle radiants arround axis.
	// axis must be normalized
	void rotateArround(float angle, glm::vec3 axis);

	glm::vec3 forward() const;
	glm::vec3 right() const;
	glm::vec3 up() const;

private:
	glm::vec3 mPos = glm::vec3(0.f);
	glm::vec3 mScale = glm::vec3(1.f);

	glm::quat mRotation = glm::quat(1.f, glm::vec3(0.f));

	// Serialization functions
	template<class Archive>
	void serialize(Archive& ar)
	{
		ar(GR_SERIALIZE_NVP_MEMBER(mPos));
		ar(GR_SERIALIZE_NVP_MEMBER(mScale));
		ar(GR_SERIALIZE_NVP_MEMBER(mRotation));
	}

	GR_SERIALIZE_PRIVATE_MEMBERS
};


} // namespace addon
} // namespace gr

GR_SERIALIZE_TYPE(gr::addon::Transform)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::addon::IAddon, gr::addon::Transform)