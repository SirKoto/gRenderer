#include "DescriptorSetLayout.h"

#include "../control/FrameContext.h"

#include <imgui/imgui.h>

namespace gr {

void DescriptorSetLayout::destroy(GlobalContext* gc)
{
    gc->rc().destroy(mDescSetLayout);
}

void DescriptorSetLayout::scheduleDestroy(FrameContext* fc)
{
    fc->scheduleToDelete(mDescSetLayout);
}

void DescriptorSetLayout::renderImGui(FrameContext* fc)
{

    ImGui::TextDisabled("Descriptor Set Layout");
    ImGui::Separator();


    ImGui::Text("Bindings:"); ImGui::SameLine();
    if (ImGui::Button(" + ")) {
        mBindings.push_back({});
    }

    int i = 0;
    for (DSL& dsl : mBindings) {
        
        vk::DescriptorSetLayoutBinding& dslb = dsl.binding;
        if (ImGui::TreeNode((void*)(intptr_t)i++, "Binding %u", dslb.binding)) {
            if (ImGui::InputInt("Binding", (int*)&dslb.binding, 1, 1)) {
                if (static_cast<int>(dslb.binding) < 0) {
                    dslb.binding = 0;
                }
            }

            constexpr std::array<vk::DescriptorType, 2> descriptortypes =
            { vk::DescriptorType::eCombinedImageSampler,
              vk::DescriptorType::eUniformBuffer };

            if (ImGui::BeginCombo("Descriptor Type", vk::to_string(dslb.descriptorType).c_str())) {
                for (int32_t i = 0; i < descriptortypes.size(); ++i) {
                    if (ImGui::Selectable(
                        vk::to_string(descriptortypes[i]).c_str(),
                        dslb.descriptorType == descriptortypes[i])) {
                        dslb.descriptorType = descriptortypes[i];
                    }

                    if (descriptortypes[i] == dslb.descriptorType) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::InputInt("Descriptor count", (int*)&dslb.descriptorCount, 1, 1)) {
                if (static_cast<int>(dslb.descriptorCount) < 0) {
                    dslb.descriptorCount = 0;
                }
            }

            constexpr std::array<vk::ShaderStageFlagBits, 3> shaderStageBits =
            { vk::ShaderStageFlagBits::eVertex,
              vk::ShaderStageFlagBits::eFragment,
              vk::ShaderStageFlagBits::eAllGraphics };

            if (ImGui::TreeNode("Shader Stages")) {

                for (vk::ShaderStageFlagBits stage : shaderStageBits) {
                    bool selected = (dslb.stageFlags & stage) == stage;
                    if (ImGui::Selectable(vk::to_string(stage).c_str(), &selected)) {
                        if (selected) {
                            dslb.stageFlags = dslb.stageFlags | stage;
                        }
                        else {
                            dslb.stageFlags = dslb.stageFlags ^ stage;
                        }
                    }
                }

                ImGui::TreePop();
            }

            bool immutableEnabled = !dsl.immutableSamplers.empty();
            if (ImGui::Checkbox("Enable immutable samplers", &immutableEnabled)) {
                if (!immutableEnabled) {
                    dsl.immutableSamplers.clear();
                }
            }
            if (immutableEnabled) {
                dsl.immutableSamplers.resize(dslb.descriptorCount, 0);

                int i = 0;
                for (ResourceDictionary::ResId id : dsl.immutableSamplers) {
                    std::string name = "Undefined";
                    if (fc->gc().getDict().exists(id)) {
                        name = fc->gc().getDict().getName(id);
                    }
                    ImGui::Text("sampler %u", i++);
                    ImGui::Button(name.c_str());
                }
            }

            ImGui::TreePop();
        }
    }
}

} // namespace gr