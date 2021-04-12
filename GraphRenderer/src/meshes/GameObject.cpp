#include "GameObject.h"

#include <imgui/imgui.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>


#include "Shader.h"
#include "../control/FrameContext.h"
#include "GameObjectAddons/Camera.h"
#include "GameObjectAddons/Renderable.h"



namespace gr
{
GameObject::GameObject(FrameContext* fc) : IObject(fc), mTransform(fc)
{
}
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

        if (ImGui::Button(addon::Camera::s_getAddonName())) {
            mAddons.emplace(addon::Camera::s_getAddonName(), new addon::Camera(fc));
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button(addon::Renderable::s_getAddonName())) {
            mAddons.emplace(addon::Renderable::s_getAddonName(), new addon::Renderable(fc));
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void GameObject::graphicsUpdate(FrameContext* fc)
{
    mTransform.updateBeforeRender(fc, this);

    for (decltype(mAddons)::iterator it = mAddons.begin(); it != mAddons.end(); ++it) {
        it->second->updateBeforeRender(fc, this);
    }
}


} // namespace gr