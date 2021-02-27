#include "DescriptorSetLayout.h"

#include "../control/FrameContext.h"

#include <imgui/imgui.h>
#include <iostream>

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

    // Create gpu info
    {
        const char* butt = mDescSetLayout ? "Update layout" : "Create layout";
        if (ImGui::Button(butt)) {
            this->markUpdated(fc);
            this->scheduleDestroy(fc);
            this->createDescriptorLayout(fc);
        }
        ImGui::Separator();
    }


    ImGui::Text("Bindings:"); ImGui::SameLine();
    if (ImGui::Button(" + ")) {
        mBindings.push_back({});
    }

    int i = 0;
    for (decltype(mBindings)::iterator it = mBindings.begin(); it != mBindings.end(); ) {
        
        vk::DescriptorSetLayoutBinding& dslb = it->binding;
        bool eraseBinding = false;
        if (ImGui::TreeNode((void*)(intptr_t)i++, "Binding %u", dslb.binding)) {

            ImGui::SameLine(); // 2 same lines to move cursor back to previous line
            // delete button aligned to the right
            ImGui::SameLine(std::max(ImGui::GetCursorPosX(), ImGui::GetContentRegionMax().x - ImGui::GetFontSize() * 3.f));
            if (ImGui::SmallButton(" - ")) {
                eraseBinding = true;
            }

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

            // if binding is sampler, and has elements
            if (dslb.descriptorType == vk::DescriptorType::eCombinedImageSampler &&
                dslb.descriptorCount > 0) {
                bool immutableEnabled = !it->immutableSamplers.empty();
                if (ImGui::Checkbox("Enable immutable samplers", &immutableEnabled)) {
                    if (!immutableEnabled) {
                        it->immutableSamplers.clear();
                    }
                }
                if (immutableEnabled) {
                    it->immutableSamplers.resize(dslb.descriptorCount, 0);

                    int i = 0;
                    for (ResId& id : it->immutableSamplers) {
                        std::string name = "Undefined";
                        if (fc->gc().getDict().exists(id)) {
                            name = fc->gc().getDict().getName(id);
                        }
                        ImGui::Text("sampler %u", i++);
                        ImGui::Button(name.c_str());
                        // This button is a drop target for a sample
                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Sampler::s_getClassName()))
                            {
                                assert(payload->DataSize == sizeof(ResId));
                                id = *reinterpret_cast<ResId*>(payload->Data);
                            }
                            ImGui::EndDragDropTarget();
                        }
                    }
                }
            }
            else {
                it->immutableSamplers.clear();
            }

            ImGui::TreePop();
        }

        if (eraseBinding) {
            it = mBindings.erase(it);
        }
        else {
            ++it;
        }
    }
}

void DescriptorSetLayout::createDescriptorLayout(FrameContext* fc)
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    std::vector<std::vector<vk::Sampler>> immutableSamplers;
    bindings.reserve(mBindings.size());

    for (const DSL& dsl : mBindings) {
        bindings.push_back(dsl.binding);

        if (!dsl.immutableSamplers.empty()) {
            immutableSamplers.push_back({});
            immutableSamplers.back().reserve(dsl.binding.descriptorCount);
            for (ResId id : dsl.immutableSamplers) {
                Sampler* sampler = nullptr;
                if (!fc->gc().getDict().exists(id)) {
                    throw std::runtime_error("Not exists sampler!");
                }
                fc->gc().getDict().get(id, &sampler);
                if (! (*sampler)) {
                    throw std::runtime_error("Sampler not initalized");
                }
                immutableSamplers.back().push_back(sampler->getVkSampler());
            }
            bindings.back().pImmutableSamplers = immutableSamplers.back().data();
        }
        else {
            bindings.back().pImmutableSamplers = nullptr;
        }
    }

    vk::DescriptorSetLayoutCreateInfo createInfo(
        {}, // flags
        bindings
    );

    mDescSetLayout = fc->rc().getDevice().createDescriptorSetLayout(createInfo);
}

DescriptorSetLayout::DSL::DSL()
{
    binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
}

} // namespace gr