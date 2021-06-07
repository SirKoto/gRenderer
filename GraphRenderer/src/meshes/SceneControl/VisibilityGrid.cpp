#include "VisibilityGrid.h"

#include <imgui/imgui.h>

namespace gr
{

VisibilityGrid::VisibilityGrid()
{
	mWallsGrid.resize(1, std::vector<uint8_t>(1, 0));
	mResolutionX = 1;
	mResolutionY = 1;
}

void VisibilityGrid::renderImGui(FrameContext* fc, Gui* gui)
{
	int32_t step = 1;
	ImGui::InputScalar("X-Resolution", ImGuiDataType_U32, (void*)&mResolutionX, &step, nullptr, "%d", ImGuiInputTextFlags_None);
	ImGui::InputScalar("Y-Resolution", ImGuiDataType_U32, (void*)&mResolutionY, &step, nullptr, "%d", ImGuiInputTextFlags_None);
	mResolutionX = (mResolutionX == 0) ? 1 : mResolutionX;
	mResolutionY = (mResolutionY == 0) ? 1 : mResolutionY;

	if (mResolutionY != (uint32_t)mWallsGrid.size() || 
		mResolutionX != (uint32_t)mWallsGrid.front().size()) {
		mWallsGrid.resize(mResolutionY);
		for (uint32_t i = 0; i < mResolutionY; ++i) {
			mWallsGrid[i].resize(mResolutionX, 0);
		}
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("Grid", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
	{
		int32_t id = 0;
		int32_t y = 0;
		for (std::vector<uint8_t>& line : mWallsGrid) {
			bool first = true;
			int32_t x = 0;
			for (uint8_t& p : line) {
				ImGui::PushID(id++);

				if (x != 0) {
					ImGui::SameLine();
				}
				else if(y % 2 == 0){
					ImGui::Selectable("", false, ImGuiSelectableFlags_Disabled, ImVec2(15, 15));
					ImGui::SameLine();
				}


				p = bool(p) != ImGui::Selectable(
					(y % 2 == 0) ? "-" : "|",
					p, 0, ImVec2(15, 15));
				ImGui::SameLine();
				ImGui::Selectable("", false, ImGuiSelectableFlags_Disabled, ImVec2(15, 15));

				ImGui::PopID();
				x += 1;
			}

			y += 1;
		}
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();


	
}

} // namespace gr