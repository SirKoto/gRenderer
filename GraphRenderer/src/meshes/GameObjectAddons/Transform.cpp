#include "Transform.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include "../../utils/math/Quaternion.h"


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
    {
        glm::vec3 euler = glm::degrees( mth::to_euler(mRotation) );
        glm::vec3 eulerCpy = euler;
        ImGui::Text("Rotation");
        ImGui::SetNextItemWidth(width);
        ImGui::DragFloat("##rot1", &euler[0], 0.05f, -FLT_MAX, FLT_MAX, "x:%.3f", ImGuiSliderFlags_NoRoundToFormat);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(width);
        ImGui::DragFloat("##rot2", &euler[1], 0.05f, -89.9f, 89.9f, "y:%.3f", ImGuiSliderFlags_NoRoundToFormat);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(width);
        ImGui::DragFloat("##rot3", &euler[2], 0.05f, -FLT_MAX, FLT_MAX, "z:%.3f", ImGuiSliderFlags_NoRoundToFormat);

        eulerCpy -= euler;
        eulerCpy = glm::abs(eulerCpy);
        if (std::max(eulerCpy.x, std::max(eulerCpy.y, eulerCpy.z)) >= 1e-3f) {
            mRotation = glm::quat(glm::radians(euler));
        }

    }


    ImGui::PopID();
}

void gr::addon::Transform::rotateArround(float angle, glm::vec3 axis)
{
    //angle = std::fmod(angle, 
    float sinA = std::sinf(angle * 0.5f);
    float cosA = std::cosf(angle * 0.5f);
    mRotation *= glm::quat(cosA, axis * sinA);

    // normalize if needed
    float dot = glm::dot(mRotation, mRotation);
    if (std::abs(dot - 1.0f) > 1e-4) {
        mRotation = mRotation / std::sqrt(dot);
    }
}

glm::vec3 gr::addon::Transform::forward() const
{
    return  glm::rotate(mRotation, glm::vec3(0.f, 0.f, 1.f));
}

glm::vec3 gr::addon::Transform::right() const
{
    return  glm::rotate(mRotation, glm::vec3(-1.f, 0.f, 0.f));
}

glm::vec3 gr::addon::Transform::up() const
{
    return  glm::rotate(mRotation, glm::vec3(0.f, 1.f, 0.f));
}
