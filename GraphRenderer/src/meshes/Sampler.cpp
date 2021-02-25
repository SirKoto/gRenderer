#include "Sampler.h"

#include "../control/FrameContext.h"

#include <imgui/imgui.h>

namespace gr {



void Sampler::destroy(GlobalContext* gc)
{
	gc->rc().destroy(mSampler);
}

void Sampler::scheduleDestroy(FrameContext* fc)
{
	fc->scheduleToDelete(mSampler);
}

void Sampler::renderImGui(FrameContext* fc)
{
	ImGui::TextDisabled("Sampler");
	ImGui::Separator();

	std::array<vk::SamplerAddressMode, 3> addressModes = 
	{ 
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eMirroredRepeat,
		vk::SamplerAddressMode::eClampToEdge
	};

	if (ImGui::BeginCombo("Address Mode", vk::to_string(mAddresMode).c_str())) {
		for (int32_t i = 0; i < addressModes.size(); ++i) {
			if (ImGui::Selectable(
				vk::to_string(addressModes[i]).c_str(),
				addressModes[i] == mAddresMode)) {
				mAddresMode = addressModes[i];
			}

			if (addressModes[i] == mAddresMode) {
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

}

} // namespace gr