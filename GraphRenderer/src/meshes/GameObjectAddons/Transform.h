#pragma once

#include "IAddon.h"

#include <glm/glm.hpp>

namespace gr {
namespace addon
{


class Transform : public IAddon
{
public:

	Transform(FrameContext* fc) : IAddon(fc) {}


	void drawImGuiInspector(FrameContext* fc, GameObject* parent) override final;

	static const char* s_getAddonName() { return "Transform"; }

	const glm::vec3& getPos() const { return mPos; }
	void setPos (const glm::vec3& pos) { mPos = pos; }
	const glm::vec3& getScale() const { return mScale; }
	void setScale(const glm::vec3& scale) { mScale = scale; }
	const glm::vec3& getRotation() const { return mRotation; }
	void setRotation(const glm::vec3& rotation) { mRotation = rotation; }


private:
	glm::vec3 mPos = glm::vec3(0.f);
	glm::vec3 mScale = glm::vec3(1.f);
	glm::vec3 mRotation = glm::vec3(0.f);
};


} // namespace addon
} // namespace gr