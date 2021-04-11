#include "Camera.h"

#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../../gui/GuiUtils.h"

namespace gr {
namespace addon {



void Camera::drawImGuiInspector(FrameContext* fc, GameObject* parent)
{
	ImGui::PushID(Camera::s_getAddonName());

	ImGui::Separator();
	ImGui::Text(Camera::s_getAddonName());

	ImGui::DragFloat("Z-Near", &mNear, 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
	
	ImGui::DragFloat("Z-Far", &mFar, 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

	ImGui::DragFloat("Fov", &mFov, 0.05f, 45.0f, 180.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
	float width = ImGui::GetWindowSize().x / 2.2f;
	ImGui::Text("Aspect ratio:");
	ImGui::SetNextItemWidth(width);
	ImGui::DragFloat("##x-aspect", &mAspectRatio.x, 0.05f, 1.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SameLine(); ImGui::Text(":"); ImGui::SameLine(); ImGui::SetNextItemWidth(width);
	ImGui::DragFloat("##y-aspect", &mAspectRatio.y, 0.05f, 1.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

	ImGui::Checkbox("Controlable", &mCameraIsControlable);
	ImGui::SameLine();
	gui::helpMarker("Make camera controlable with mouse and keyboard, when mouse right click.");

	ImGui::PopID();
}

void Camera::preRenderUpdate(FrameContext* fc, GameObject* parent)
{
	glm::mat4 V = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat3 P = glm::perspective(mFov,
		mAspectRatio.x / mAspectRatio.y,
		mNear, mFar);
	P[1][1] *= -1.0;
}

} // namespace addon
} // namespace gr