#include "Camera.h"

#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../../gui/GuiUtils.h"
#include "../../control/FrameContext.h"
#include "../Scene.h"

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

	ImGui::PopID();
}

std::unique_ptr<addon::IAddon> Camera::duplicate(FrameContext* fc, const GameObject* parent) const
{
    Camera* nc = new Camera();
    nc->start(fc);
    
    nc->mFov = this->mFov;
    nc->mFar = this->mFar;
    nc->mNear = this->mNear;
    nc->mAspectRatio = this->mAspectRatio;

    return std::unique_ptr<addon::IAddon>(nc);
}

void Camera::updateBeforeRender(FrameContext* fc, GameObject* parent, const SceneRenderContext& src)
{
    fc->renderSubmitter().setSceneDescriptorSet(mCameraDescriptorSets[fc->getIdx()]);

    const Transform* transform = parent->getAddon<Transform>();

    vkg::RenderContext::BasicCameraTransformUBO ubo;

    ubo.V = glm::lookAt(transform->getPos(), transform->getPos() + transform->forward(),
		transform->up());
    ubo.P = glm::perspective( glm::radians(mFov),
		mAspectRatio.x / mAspectRatio.y,
		mNear, mFar);
    ubo.P[1][1] *= -1.0;


    size_t sizePadd = fc->rc().padUniformBuffer(sizeof(vkg::RenderContext::BasicCameraTransformUBO));

    std::memcpy(mUbosGpuPtr + sizePadd * fc->getIdx(), &ubo, sizeof(vkg::RenderContext::BasicCameraTransformUBO));

}

void Camera::destroy(FrameContext* fc)
{
    if (mUbosGpuPtr) {
        fc->rc().unmapAllocatable(mUbos);
        mUbosGpuPtr = nullptr;
    }

    if (mUbos) {
        fc->scheduleToDestroy(mUbos);
    }

    for (vk::DescriptorSet set : mCameraDescriptorSets) {
        fc->rc().freeDescriptorSet(set, fc->rc().getBasicCameraTransformLayout());
    }
}

void Camera::createUbos(FrameContext* fc)
{
    if (mUbos) {
        return;
    }
    const uint32_t concurrentFrames = fc->getNumConcurrentFrames();

    size_t sizePadded = fc->rc().padUniformBuffer(sizeof(vkg::RenderContext::BasicCameraTransformUBO));
    mUbos = fc->rc().createUniformBuffer(
        concurrentFrames * sizePadded
    );

    fc->rc().mapAllocatable(mUbos, reinterpret_cast<void**>(&mUbosGpuPtr));


    // allocate descriptor sets
    mCameraDescriptorSets.resize(concurrentFrames);
    fc->rc().allocateDescriptorSet(
        concurrentFrames,                   // num
        fc->rc().getBasicCameraTransformLayout(), // descriptor set layout
        mCameraDescriptorSets.data());      // out
    // write
    for (uint32_t i = 0; i < concurrentFrames; ++i) {
        vk::DescriptorBufferInfo buffInfo(
            mUbos.getVkBuffer(),            // buffer
            sizePadded * i,                 // offset
            sizeof(vkg::RenderContext::BasicCameraTransformUBO) // range
        );

        vk::WriteDescriptorSet write(
            mCameraDescriptorSets[i],   // dst descriptor set
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