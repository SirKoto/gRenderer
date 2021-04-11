#include "Transform.h"

#include <imgui/imgui.h>


void gr::addon::Transform::drawImGuiInspector(FrameContext* fc, GameObject* parent)
{

    ImGui::PushID(Transform::s_getAddonName());

    ImGui::Separator();
    ImGui::Text(Transform::s_getAddonName());

    float width = ImGui::GetWindowSize().x / 3.5f;
    // position
    ImGui::Text("Position");
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##pos1", &mPos[0], 0.05f, -FLT_MAX, FLT_MAX, "x:%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##pos2", &mPos[1], 0.05f, -FLT_MAX, FLT_MAX, "y:%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##pos3", &mPos[2], 0.05f, -FLT_MAX, FLT_MAX, "z:%.3f", ImGuiSliderFlags_NoRoundToFormat);

    // scale
    ImGui::Text("Scale");
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##scale1", &mScale[0], 0.05f, -FLT_MAX, FLT_MAX, "x:%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##scale2", &mScale[1], 0.05f, -FLT_MAX, FLT_MAX, "y:%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##scale3", &mScale[2], 0.05f, -FLT_MAX, FLT_MAX, "z:%.3f", ImGuiSliderFlags_NoRoundToFormat);

    // rotation
    ImGui::Text("Rotation");
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##rot1", &mRotation[0], 0.05f, -FLT_MAX, FLT_MAX, "x:%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##rot2", &mRotation[1], 0.05f, -FLT_MAX, FLT_MAX, "y:%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##rot3", &mRotation[2], 0.05f, -FLT_MAX, FLT_MAX, "z:%.3f", ImGuiSliderFlags_NoRoundToFormat);

    ImGui::PopID();
}
