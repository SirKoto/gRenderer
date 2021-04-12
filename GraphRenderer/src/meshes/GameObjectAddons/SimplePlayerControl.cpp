#include "SimplePlayerControl.h"

#include <imgui/imgui.h>
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


	ImGui::PopID();
}

void SimplePlayerControl::update(FrameContext* fc, GameObject* parent)
{
	const vkg::Window& w = fc->gc().getWindow();
	if (!w.isDown(vkg::Window::Input::MouseRight)) {
		return;
	}

	if (w.isDown(vkg::Window::Input::KeyW)) {

	}


}

} // namespace addon
} // namespace gr