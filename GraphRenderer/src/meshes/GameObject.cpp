#include "GameObject.h"

#include <imgui/imgui.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>


#include "Shader.h"
#include "../control/FrameContext.h"
#include "GameObjectAddons/Camera.h"
#include "GameObjectAddons/Renderable.h"
#include "GameObjectAddons/SimplePlayerControl.h"



namespace gr
{

void GameObject::scheduleDestroy(FrameContext* fc)
{
    mTransform.destroy(fc);

    for (decltype(mAddons)::iterator it = mAddons.begin(); it != mAddons.end(); ++it) {
        it->second->destroy(fc);
    }
}

void GameObject::renderImGui(FrameContext* fc, GuiFeedback* feedback)
{

    ImGui::TextDisabled("GameObject");

    // ADDONS
    ImGui::Separator();

    mTransform.drawImGuiInspector(fc, this);

    for (decltype(mAddons)::iterator it = mAddons.begin(); it != mAddons.end(); ++it) {
        it->second->drawImGuiInspector(fc, this);
    }

    ImGui::Separator();
    ImGui::Button("Append addon");
    if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_MouseButtonLeft)) {
        std::pair<decltype(mAddons)::iterator, bool> insertRes;
        if (ImGui::Button(addon::Camera::s_getAddonName())) {
            insertRes = mAddons.emplace(addon::Camera::s_getAddonName(), new addon::Camera());
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button(addon::Renderable::s_getAddonName())) {
            insertRes = mAddons.emplace(addon::Renderable::s_getAddonName(), new addon::Renderable());
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button(addon::SimplePlayerControl::s_getAddonName())) {
            insertRes = mAddons.emplace(addon::SimplePlayerControl::s_getAddonName(), new addon::SimplePlayerControl());
            ImGui::CloseCurrentPopup();
        }
        
        if (insertRes.second) {
            insertRes.first->second->start(fc);
        }

        ImGui::EndPopup();
    }
}

void GameObject::start(FrameContext* fc)
{
    for (decltype(mAddons)::iterator it = mAddons.begin(); it != mAddons.end(); ++it) {
        it->second->start(fc);
    }
    mTransform.start(fc);
}

void GameObject::graphicsUpdate(FrameContext* fc, const SceneRenderContext& src)
{
    mTransform.updateBeforeRender(fc, this, src);

    for (decltype(mAddons)::iterator it = mAddons.begin(); it != mAddons.end(); ++it) {
        it->second->updateBeforeRender(fc, this, src);
    }
}

void GameObject::logicUpdate(FrameContext* fc)
{
    mTransform.update(fc, this);

    for (decltype(mAddons)::iterator it = mAddons.begin(); it != mAddons.end(); ++it) {
        it->second->update(fc, this);
    }
}


} // namespace gr