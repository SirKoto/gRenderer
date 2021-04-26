#include "Renderable.h"

#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../Mesh.h"
#include "../../control/FrameContext.h"


namespace gr
{
namespace addon
{

void Renderable::drawImGuiInspector(FrameContext* fc, GameObject* parent)
{
    ImGui::PushID(Renderable::s_getAddonName());

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


    ImGui::PopID();
}

void Renderable::updateBeforeRender(FrameContext* fc, GameObject* parent, const SceneRenderContext& src)
{

    Transform* transf = parent->getAddon<Transform>();
    assert(transf != nullptr);

    // update UBO
    {
        vkg::RenderContext::BasicTransformUBO ubo;
        ubo.M = glm::mat4(1.0);

        ubo.M = glm::translate(ubo.M, transf->getPos());
        ubo.M = ubo.M * glm::mat4_cast(transf->getRotation());
        ubo.M = glm::scale(ubo.M, transf->getScale());

        size_t sizePadd = fc->rc().padUniformBuffer(sizeof(vkg::RenderContext::BasicTransformUBO));

        std::memcpy(mUbosGpuPtr + sizePadd * fc->getIdx(), &ubo, sizeof(vkg::RenderContext::BasicTransformUBO));
    }

    // get mesh and schedule draw
    if (this->mMesh) {
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

void Renderable::destroy(FrameContext* fc)
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

void Renderable::start(FrameContext* fc)
{
    createUbos(fc);
}

void Renderable::setMesh(ResId meshId)
{
	mMesh = meshId;
}

void Renderable::createUbos(FrameContext* fc)
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

} // namespace addon
} // namespace gr
