#include "VisibilityGrid.h"

#include <imgui/imgui.h>
#include <filesystem>

namespace gr
{

VisibilityGrid::VisibilityGrid()
{
	mWallsGrid.resize(1, std::vector<Cell>(1));
	mResolutionX = 1;
	mResolutionY = 1;

	mMesh = std::make_unique<Mesh>();

}

void VisibilityGrid::start(FrameContext* fc)
{
	mMesh->start(fc);
	std::filesystem::path p = std::filesystem::current_path() / "resources" / "models" / "plane.ply";
	mMesh->load(fc, p.string().c_str());
}

void VisibilityGrid::scheduleDestroy(FrameContext* fc)
{
	mMesh->scheduleDestroy(fc);
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
			mWallsGrid[i].resize(mResolutionX);
		}
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("Grid", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
	{
		int32_t id = 0;
		
		for (uint32_t y = 0; y < mResolutionY; ++y) {
			std::vector<Cell>& line = mWallsGrid[y];
			
			for (uint32_t x = 0; x < mResolutionX; ++x) {
				Cell& c = line[x];
				ImGui::PushID(id++);

				ImGui::Selectable(".", false, ImGuiSelectableFlags_Disabled, ImVec2(15, 15));
				ImGui::SameLine();

				c.up() = bool(c.up()) != ImGui::Selectable(
					 "-",
					c.up(), 0, ImVec2(15, 15));
				ImGui::SameLine();

				ImGui::PopID();
			}

			ImGui::Selectable(".", false, ImGuiSelectableFlags_Disabled, ImVec2(15, 15));

			for (uint32_t x = 0; x < mResolutionX; ++x) {
				Cell& c = line[x];
				ImGui::PushID(id++);

				c.left() = bool(c.left()) != ImGui::Selectable(
					"|",
					c.left(), 0, ImVec2(15, 15));
				ImGui::SameLine();
				ImGui::Selectable("", false, ImGuiSelectableFlags_Disabled, ImVec2(15, 15));
				ImGui::SameLine();

				ImGui::PopID();
			}

			ImGui::PushID(id++);
			line.back().right() = bool(line.back().right()) != ImGui::Selectable(
				"|",
				line.back().right(), 0, ImVec2(15, 15));
			ImGui::PopID();
		}

		// bottom line
		for (uint32_t x = 0; x < mResolutionX; ++x) {
			Cell& c = mWallsGrid.back()[x];
			ImGui::PushID(id++);

			ImGui::Selectable(".", false, ImGuiSelectableFlags_Disabled, ImVec2(15, 15));
			ImGui::SameLine();

			c.down() = bool(c.down()) != ImGui::Selectable(
				"-",
				c.down(), 0, ImVec2(15, 15));
			ImGui::SameLine();

			ImGui::PopID();
		}
		ImGui::Selectable(".", false, ImGuiSelectableFlags_Disabled, ImVec2(15, 15));

	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
	
	// TODO: propagate changes to neighbors
}

VisibilityGrid::Cell::Cell()
{
	occupied.fill(0);
}

} // namespace gr