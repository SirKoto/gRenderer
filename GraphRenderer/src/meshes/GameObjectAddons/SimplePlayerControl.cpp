#include "SimplePlayerControl.h"

#include <imgui/imgui.h>
#include <iostream>
#include "../../control/FrameContext.h"

namespace gr
{
namespace addon {

void SimplePlayerControl::drawImGuiInspector(FrameContext* fc, GameObject* parent)
{
	ImGui::PushID(SimplePlayerControl::s_getAddonName());

	ImGui::Separator();
	ImGui::Text(SimplePlayerControl::s_getAddonName());

	ImGui::DragFloat("Speed", &mSpeed, 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
	ImGui::DragFloat("Mouse Speed", &mMouseSpeed, 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);


	ImGui::PopID();
}

void SimplePlayerControl::update(FrameContext* fc, GameObject* parent)
{
	const vkg::Window& w = fc->gc().getWindow();
	if (!w.isDown(vkg::Window::Input::MouseRight)) {
		return;
	}

	Transform* trns = parent->getAddon<Transform>();
	assert(trns != nullptr);
	glm::vec3 pos = trns->getPos();
	if (w.isDown(vkg::Window::Input::KeyW)) {
		pos += fc->dtf() * mSpeed * trns->forward();
	}
	if (w.isDown(vkg::Window::Input::KeyS)) {
		pos -= fc->dtf() * mSpeed * trns->forward();
	}
	if (w.isDown(vkg::Window::Input::KeyD)) {
		pos += fc->dtf() * mSpeed * trns->right();
	}
	if (w.isDown(vkg::Window::Input::KeyA)) {
		pos -= fc->dtf() * mSpeed * trns->right();
	}

	trns->setPos(pos);
	std::array<double, 2> newMousePos;
	w.getMousePosition(&newMousePos);
	if (newMousePos[0] >= 0.f) {
		glm::vec2 mousePos(newMousePos[0], newMousePos[1]);
		if ( w.isJustPressed(vkg::Window::Input::MouseRight)) {
			mPreMousePos = mousePos;
		}
		glm::vec2 dPos = mousePos - mPreMousePos;
		if (std::abs(dPos.x) > 1e-5) {
			trns->rotateArround( glm::radians(dPos.x * mMouseSpeed), glm::vec3(0, 1, 0));
		}
		if (std::abs(dPos.y) > 1e-5) {
			trns->rotateArround(glm::radians(dPos.y * mMouseSpeed), trns->right());
		}

		mPreMousePos = mousePos;
	}

}

} // namespace addon
} // namespace gr