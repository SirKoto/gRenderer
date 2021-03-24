#include "Pipeline.h"

#include "../control/FrameContext.h"

#include <imgui/imgui.h>

namespace gr
{
void Pipeline::destroy(GlobalContext* gc)
{
	gc->rc().destroy(mPipeline);
	gc->rc().destroy(mPipelineLayout);
}
void Pipeline::scheduleDestroy(FrameContext* fc)
{
	fc->scheduleToDelete(mPipeline);
	fc->scheduleToDelete(mPipelineLayout);
}
void Pipeline::renderImGui(FrameContext* fc)
{
	ImGui::TextDisabled("Pipeline");
	ImGui::Separator();
	// Create gpu info
	{
		const char* butt = mPipeline ? "Update pipeline" : "Create pipeline";
		if (ImGui::Button(butt)) {
			this->signalNeedsLatterUpdate();
		}
		ImGui::Separator();
	}

	// Shader stages
	{
		ImGui::Text("Shader stages");
		bool minus = ImGui::Button("-"); ImGui::SameLine();
		ImGui::TextDisabled("%u", static_cast<uint32_t>(mShaderStages.size())); ImGui::SameLine();
		bool plus = ImGui::Button("+");

		if (minus && !mShaderStages.empty()) {
			mShaderStages.resize(mShaderStages.size() - 1);
		}
		if (plus) {
			mShaderStages.resize(mShaderStages.size() + 1);
		}

		for (const ResId& id : mShaderStages) {

		}

	}


}

void Pipeline::updateInternal(FrameContext* fc)
{

}

} // namespace gr