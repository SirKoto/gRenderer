#include "VisibilityGrid.h"

#include <imgui/imgui.h>
#include <filesystem>

#include "../GameObjectAddons/Renderable.h"
#include "../../control/FrameContext.h"
namespace gr
{

VisibilityGrid::VisibilityGrid()
{
	mWallsCells.resize(1, std::vector<Cell>(1));
	mResolutionX = 1;
	mResolutionY = 1;

}

void VisibilityGrid::start(FrameContext* fc)
{
	Mesh* mesh;
	mMesh = fc->gc().getDict().allocateObject(fc, "wall", &mesh);
	mesh->start(fc);
	std::filesystem::path p = std::filesystem::current_path() / "resources" / "models" / "plane.ply";
	mesh->load(fc, p.string().c_str());
}

void VisibilityGrid::scheduleDestroy(FrameContext* fc)
{
	fc->gc().getDict().erase(mMesh);
}

void VisibilityGrid::renderImGui(FrameContext* fc, Gui* gui)
{
	int32_t step = 1;
	ImGui::InputScalar("X-Resolution", ImGuiDataType_U32, (void*)&mResolutionX, &step, nullptr, "%d", ImGuiInputTextFlags_None);
	ImGui::InputScalar("Y-Resolution", ImGuiDataType_U32, (void*)&mResolutionY, &step, nullptr, "%d", ImGuiInputTextFlags_None);
	mResolutionX = (mResolutionX == 0) ? 1 : mResolutionX;
	mResolutionY = (mResolutionY == 0) ? 1 : mResolutionY;

	if (mResolutionY != (uint32_t)mWallsCells.size() || 
		mResolutionX != (uint32_t)mWallsCells.front().size()) {
		mWallsCells.resize(mResolutionY);
		for (uint32_t i = 0; i < mResolutionY; ++i) {
			mWallsCells[i].resize(mResolutionX);
		}
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("Grid", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
	{
		int32_t id = 0;
		
		for (uint32_t y = 0; y < mResolutionY; ++y) {
			std::vector<Cell>& line = mWallsCells[y];
			
			for (uint32_t x = 0; x < mResolutionX; ++x) {
				Cell& c = line[x];
				ImGui::PushID(id++);

				ImGui::Selectable(".", false, ImGuiSelectableFlags_Disabled, ImVec2(15, 15));
				ImGui::SameLine();

				c.up() = bool(c.up()) != ImGui::Selectable(
					 "-",
					c.up(), 0, ImVec2(15, 15));
				ImGui::SameLine();

				// update also upper cell
				if (y > 0) {
					mWallsCells[y - 1][x].down() = c.up();
				}

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

				// update also left cell
				if (x > 0) {
					line[x - 1].right() = c.left();
				}

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
			Cell& c = mWallsCells.back()[x];
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

	for (uint32_t y = 0; y < mResolutionY; ++y) {
		for (uint32_t x = 0; x < mResolutionX; ++x) {
			updateWallCellGameObject(fc, x, y);
		}
	}
}

void VisibilityGrid::updateWallCellGameObject(FrameContext* fc, uint32_t x, uint32_t y)
{
	const Cell& c = mWallsCells[y][x];

	auto updateSingleWall = [this, fc](uint32_t x, uint32_t y, bool vertical, bool isWallEnabled) {
		decltype(mWallsGameObjects)::iterator it;
		it = mWallsGameObjects.find(WallKey{ x, y, vertical });
		if (it == mWallsGameObjects.end() && isWallEnabled) {
			// instantiate game object
			std::unique_ptr<GameObject> obj = std::make_unique<GameObject>();
			obj->addAddon<addon::Renderable>(fc);
			addon::Renderable* rnd = obj->getAddon<addon::Renderable>();
			rnd->setMesh(mMesh);

			obj->getAddon<addon::Transform>()->setPos(glm::vec3(x, 0, y));
			if (vertical) {
				obj->getAddon<addon::Transform>()->rotateArround(glm::pi<float>(), glm::vec3(0,1,0));
			}

		}
		else if (it != mWallsGameObjects.end() && isWallEnabled == 0) {
			// destroy object
			it->second->scheduleDestroy(fc);
			mWallsGameObjects.erase(it);
		}
	};
	
	updateSingleWall(x    , y    , false, c.up());
	updateSingleWall(x	  , y    , true, c.left());
	updateSingleWall(x + 1, y    , true, c.right());
	updateSingleWall(x    , y + 1, false, c.down());


}


VisibilityGrid::Cell::Cell()
{
	occupied.fill(0);
}

} // namespace gr