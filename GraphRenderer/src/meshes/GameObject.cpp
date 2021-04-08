#include "GameObject.h"

#include <imgui/imgui.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>


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

    for (vk::DescriptorSet set : mObjectDescriptorSets) {
        fc->rc().freeDescriptorSet(set, fc->rc().getBasicTransformLayout());
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
    ImGui::DragFloat("##pos1", &mPos[0], 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##pos2", &mPos[1], 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##pos3", &mPos[2], 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

    // scale
    ImGui::Text("Scale");
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##scale1", &mScale[0], 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##scale2", &mScale[1], 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##scale3", &mScale[2], 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

    // rotation
    ImGui::Text("Rotation");
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##rot1", &mRotation[0], 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##rot2", &mRotation[1], 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::DragFloat("##rot3", &mRotation[2], 0.05f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

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

void GameObject::graphicsUpdate(FrameContext* fc)
{
    if (!mUbos) {
        createUbos(fc);
    }

    // update UBO
    {
        vkg::RenderContext::BasicTransformUBO ubo;
        ubo.M = glm::mat4(1.0);
        ubo.P = glm::mat4(1.0);
        ubo.V = glm::mat4(1.0);

        ubo.M = glm::rotate(ubo.M, mRotation.x, glm::vec3(1.f, 0.f, 0.f));
        ubo.M = glm::rotate(ubo.M, mRotation.y, glm::vec3(0.f, 1.f, 0.f));
        ubo.M = glm::rotate(ubo.M, mRotation.z, glm::vec3(0.f, 0.f, 1.f));
        ubo.M = glm::scale(ubo.M, this->mScale);
        ubo.M = glm::translate(ubo.M, this->mPos);

        size_t sizePadd = fc->rc().padUniformBuffer(sizeof(vkg::RenderContext::BasicTransformUBO));

        std::memcpy(mUbosGpuPtr + sizePadd * fc->getIdx(), &ubo, sizeof(vkg::RenderContext::BasicTransformUBO));
    }

    // get mesh and schedule draw
    if(this->mMesh) {
        Mesh* mesh;
        fc->gc().getDict().get(mMesh, &mesh);

        if (*mesh) {

            vkg::RenderSubmitter::DrawData drawData;
            drawData.vertexBuffer = mesh->getVB();
            drawData.indexBuffer = mesh->getIB();
            drawData.numIndices = mesh->getNumIndices();
            drawData.objectDescriptorSet = mObjectDescriptorSets[fc->getIdx()];

            fc->renderSubmitter().pushPredefinedDraw(drawData);
        }

    }
}

void GameObject::createUbos(FrameContext* fc)
{
    if (mUbos) {
        return;
    }
    const uint32_t concurrentFrames = fc->getNumConcurrentFrames();

    size_t sizePadded = fc->rc().padUniformBuffer(sizeof(vkg::RenderContext::BasicTransformUBO));
    mUbos = fc->rc().createUniformBuffer(
        concurrentFrames * sizePadded
    );

    fc->rc().mapAllocatable(mUbos, reinterpret_cast<void**>(&mUbosGpuPtr));


    // allocate descriptor sets
    mObjectDescriptorSets.resize(concurrentFrames);
    fc->rc().allocateDescriptorSet(
        concurrentFrames,                   // num
        fc->rc().getBasicTransformLayout(), // descriptor set layout
        mObjectDescriptorSets.data());      // out
    // write
    for (uint32_t i = 0; i < concurrentFrames; ++i) {
        vk::DescriptorBufferInfo buffInfo(
            mUbos.getVkBuffer(),            // buffer
            sizePadded * i,                 // offset
            sizeof(vkg::RenderContext::BasicTransformUBO) // range
        );

        vk::WriteDescriptorSet write(
            mObjectDescriptorSets[i],   // dst descriptor set
            0, 0,                       // dst binding, dst array
            1,                          // descriptor count
            vk::DescriptorType::eUniformBuffer,
            nullptr, &buffInfo, nullptr
        );

        fc->rc().getDevice().updateDescriptorSets(1, &write, 0, nullptr);
    }

}


} // namespace gr