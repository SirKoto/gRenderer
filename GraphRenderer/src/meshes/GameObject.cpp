#include "GameObject.h"

#include <imgui/imgui.h>
#include "Shader.h"

#include "../control/FrameContext.h"

namespace gr
{


void GameObject::scheduleDestroy(FrameContext* fc)
{
    if (mUbosGpuPtr) {
        fc->rc().unmapAllocatable(mUbos);
        mUbosGpuPtr = nullptr;
    }

    if (mUbos) {
        fc->scheduleToDestroy(mUbos);
    }
}

void GameObject::renderImGui(FrameContext* fc, GuiFeedback* feedback)
{

    ImGui::TextDisabled("GameObject");
    // TRANSFORM
    ImGui::Separator();
    
    float width = ImGui::GetWindowSize().x / 3.5f;
    // position
    ImGui::Text("Position");
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##pos1", &mPos[0], 0.5f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##pos2", &mPos[1], 0.5f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##pos3", &mPos[2], 0.5f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

    // scale
    ImGui::Text("Scale");
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##scale1", &mScale[0], 0.5f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##scale2", &mScale[1], 0.5f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##scale3", &mScale[2], 0.5f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

    // rotation
    ImGui::Text("Rotation");
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##rot1", &mRotation[0], 0.5f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##rot2", &mRotation[1], 0.5f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##rot3", &mRotation[2], 0.5f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

    // TRIANGLE MESH
    ImGui::Separator();

    std::string name = "Undefined";
    if (mMesh) {
        if (fc->gc().getDict().exists(mMesh)) {
            name = fc->gc().getDict().getName(mMesh);
        }
        else {
            mMesh.reset();
        }
    }


    ImGui::Text("Mesh:");
    ImGui::Button(name.c_str());
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Mesh::s_getClassName()))
        {
            assert(payload->DataSize == sizeof(ResId));
            ResId id = *reinterpret_cast<ResId*>(payload->Data);

            this->setMesh(id);
        }
        ImGui::EndDragDropTarget();
    }
}

void GameObject::createUbos(FrameContext* fc)
{
    if (mUbosGpuPtr) {
        fc->rc().unmapAllocatable(mUbos);
        mUbosGpuPtr = nullptr;
    }
    if (mUbos) {
        fc->scheduleToDestroy(mUbos);
    }

    mUbos = fc->rc().createUniformBuffer(
        fc->getNumConcurrentFrames() * sizeof(Shader::transformUBO)
    );

    fc->rc().mapAllocatable(mUbos, &mUbosGpuPtr);
}


} // namespace gr