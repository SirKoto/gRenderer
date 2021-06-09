#include "VisibilityGrid.h"

#include <imgui/imgui.h>
#include <filesystem>
#include <queue>
#include <chrono>
#include <sstream>

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
	Mesh* mesh = nullptr;
	if (fc->gc().getDict().existsName("wall_visibility_grid")) {
		mMesh = fc->gc().getDict().getId("wall_visibility_grid");
		fc->gc().getDict().get(mMesh, &mesh);
	}
	else {
		mMesh = fc->gc().getDict().allocateObject(fc, "wall_visibility_grid", &mesh);
	}
	mesh->start(fc);
	std::filesystem::path p = std::filesystem::current_path() / "resources" / "models" / "plane.ply";
	mesh->load(fc, p.string().c_str());

	for (uint32_t y = 0; y < mResolutionY; ++y) {
		for (uint32_t x = 0; x < mResolutionX; ++x) {
			updateWallCellGameObject(fc, x, y);
		}
	}
}

void VisibilityGrid::scheduleDestroy(FrameContext* fc)
{
	fc->gc().getDict().erase(mMesh);

	for (auto& it : mWallsGameObjects) {
		it.second->scheduleDestroy(fc);
	}
	mWallsGameObjects.clear();
}

void VisibilityGrid::renderImGui(FrameContext* fc, Gui* gui)
{
	ImGui::PushID("VisibilityGrid");

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
	ImGui::BeginChild("Grid", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
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

	ImGui::PopID();
}

void VisibilityGrid::graphicsUpdate(FrameContext* fc, const SceneRenderContext& src)
{
	for (auto& it : mWallsGameObjects) {
		it.second->graphicsUpdate(fc, src);
	}
}

void VisibilityGrid::logicUpdate(FrameContext* fc)
{
	for (auto& it : mWallsGameObjects) {
		it.second->logicUpdate(fc);
	}
}

void VisibilityGrid::computeVisibility(FrameContext* fc, const std::set<ResId>& gameObjects)
{
	const auto start_timer = std::chrono::high_resolution_clock::now();

	// create tmp rasterization of the gameobjects of the scene
	std::vector<std::vector<std::set<ResId>>> objectsRasterized(mResolutionY, std::vector<std::set<ResId>>(mResolutionX));
	// Rebuild visibility grid
	mVisibilityGrid = std::vector<std::vector<std::set<ResId>>>(mResolutionY, std::vector<std::set<ResId>>(mResolutionX));
	// rasterize all gameobjects in axis aligned grid
	for (const ResId& id : gameObjects) {
		GameObject* obj;
		fc->gc().getDict().get(id, &obj);
		gr::mth::AABBox bb = obj->getRenderBB(fc);
		glm::ivec2 from = glm::floor(glm::vec2(bb.getMin().x, bb.getMin().z));
		glm::ivec2 to = glm::floor(glm::vec2(bb.getMax().x, bb.getMax().z));
		from = glm::max(from, glm::ivec2(0, 0));
		to = glm::min(to, glm::ivec2(mResolutionX, mResolutionY) - 1);
		for (int32_t j = from.y; j <= to.y; ++j) {
			for (int32_t i = from.x; i <= to.x; ++i) {
				objectsRasterized[j][i].insert(id);
			}
		}
	}

	std::set<ResId> gameObjectsInLine;
	std::vector<glm::ivec2> cellsInLine;
	constexpr uint32_t SAMPLES_PER_CELL = 1000;


	// origin rays from all cells, even if it is redundant
	for (uint32_t j = 0; j < mResolutionY; ++j) {
		for (uint32_t i = 0; i < mResolutionX; ++i) {
			for (uint32_t sample = 0; sample < SAMPLES_PER_CELL; ++sample) {
				cellsInLine.clear();
				// bresenham line algorithm
				{
					// start by adding itself
					cellsInLine.push_back({ i, j });
					// Compute line direction TODO: precompute
					const float alpha = 2.f * glm::pi<float>() * sample / float(SAMPLES_PER_CELL - 1);
					glm::vec2 dir = glm::vec2(std::cosf(alpha), std::sinf(alpha));

					glm::ivec2 step = glm::sign(dir);
					float error = 0.0f;
					float errorprev = 0.0f;
					glm::ivec2 pos(i, j);
					if (std::abs(dir.x) >= std::abs(dir.y)) {
						errorprev = error = dir.x * 0.5f; // start in the "middle"
						while (true) {
							// check if can advance in the x direction
							bool canGoThrough = (step.x >= 0) ?
								!mWallsCells[pos.y][pos.x].right() :
								!mWallsCells[pos.y][pos.x].left();
							// advance
							pos.x += step.x;
							if (!canGoThrough || pos.x < 0 || pos.x >= (int32_t)mResolutionX) {
								break; // exit
							}
							error += dir.y; // add error in y
							if (error > dir.x) { // if error is greater that dx
								canGoThrough = (step.y >= 0) ?
									!mWallsCells[pos.y][pos.x].down() :
									!mWallsCells[pos.y][pos.x].up();
								pos.y += step.y;
								if (!canGoThrough || pos.y < 0 || pos.y >= (int32_t)mResolutionY) {
									break; // exit
								}
								error -= dir.x; // remove corrected error
								// 3 cases
								const Cell& c = mWallsCells[pos.y][pos.x];
								const bool canVert = (step.y >= 0 ? !c.down() : !c.up()) &&
									(pos.y - step.y) >= 0 &&
									(pos.y - step.y) < (int32_t)mResolutionY;
								const bool canHoriz = (step.x >= 0 ? !c.right() : !c.left()) &&
									(pos.x - step.x) >= 0 &&
									(pos.x - step.x) < (int32_t)mResolutionX;
								if (error + errorprev < dir.x) { // bottom
									if (canVert) {
										cellsInLine.push_back({ pos.x, pos.y - step.y });
									}
								}
								else if (error + errorprev > dir.x) { // left
									if (canHoriz) {
										cellsInLine.push_back({ pos.x - step.x, pos.y });
									}
								}
								else {
									if (canVert) {
										cellsInLine.push_back({ pos.x, pos.y - step.y });
									}
									if (canHoriz) {
										cellsInLine.push_back({ pos.x - step.x, pos.y });
									}
								}

							}
							cellsInLine.push_back({ pos.x, pos.y });
							errorprev = error;
						}
					}
					else { // The same as the above
						errorprev = error = dir.y * 0.5f; // start in the "middle"
						while (true) {
							// check if can advance in the y direction
							bool canGoThrough = (step.y >= 0) ?
								!mWallsCells[pos.y][pos.x].down() :
								!mWallsCells[pos.y][pos.x].up();
							// advance
							pos.y += step.y;
							if (!canGoThrough || pos.y < 0 || pos.y >= (int32_t)mResolutionY) {
								break; // exit
							}
							error += dir.x; // add error in x
							if (error > dir.y) { // if error is greater that dy
								canGoThrough = (step.x >= 0) ?
									!mWallsCells[pos.y][pos.x].right() :
									!mWallsCells[pos.y][pos.x].left();
								pos.x += step.x;
								if (!canGoThrough || pos.x < 0 || pos.x >= (int32_t)mResolutionX) {
									break; // exit
								}
								error -= dir.y; // remove corrected error
								// 3 cases
								const Cell& c = mWallsCells[pos.y][pos.x];
								const bool canVert = (step.y >= 0 ? !c.down() : !c.up()) &&
									(pos.y - step.y) >= 0 &&
									(pos.y - step.y) < (int32_t)mResolutionY;
								const bool canHoriz = (step.x >= 0 ? !c.right() : !c.left()) &&
									(pos.x - step.x) >= 0 &&
									(pos.x - step.x) < (int32_t)mResolutionX;
								if (error + errorprev < dir.y) {
									if (canHoriz) {
										cellsInLine.push_back({ pos.x - step.x, pos.y });
									}
								}
								else if (error + errorprev > dir.y) {
									if (canVert) {
										cellsInLine.push_back({ pos.x, pos.y - step.y });
									}
								}
								else {
									if (canVert) {
										cellsInLine.push_back({ pos.x, pos.y - step.y });
									}
									if (canHoriz) {
										cellsInLine.push_back({ pos.x - step.x, pos.y });
									}
								}

							}
							cellsInLine.push_back({ pos.x, pos.y });
							errorprev = error;
						}
					}
				}
				
				// accumulate information of the line
				gameObjectsInLine.clear();
				for (const glm::ivec2& v : cellsInLine) {
					const auto& o = objectsRasterized[v.y][v.x];
					gameObjectsInLine.insert(o.begin(), o.end());
				}

				// add to visibility grid
				for (const glm::ivec2& v : cellsInLine) {
					mVisibilityGrid[v.y][v.x].insert(gameObjectsInLine.begin(), gameObjectsInLine.end());
				}
			}
		}
	}

	// Log duration
	const auto end_timer = std::chrono::high_resolution_clock::now();
	typedef std::chrono::duration<double_t> Fsec;

	Fsec dur = end_timer - start_timer;
	std::stringstream ss;
	ss << "Created Cell Visibillity for scene " << this->getObjectName() << '\n';
	ss << "\tTook " << dur.count() << " seconds\n";
	fc->gc().addNewLog(ss.str());
}

std::set<ResId> g_stupid_set;
const std::set<ResId>& VisibilityGrid::getVisibleSet(const glm::vec3& pos) const
{
	int32_t x = (int32_t)std::floor(pos.x);
	int32_t y = (int32_t)std::floor(pos.z);
	if (x < 0 || y < 0 || y >= (int32_t)mVisibilityGrid.size() || x >= (int32_t)mVisibilityGrid.front().size()) {
		return g_stupid_set;
	}
	return mVisibilityGrid[y][x];
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
				obj->getAddon<addon::Transform>()->rotateArround(0.5f * glm::pi<float>(), glm::vec3(0,1,0));
			}
			else {
				obj->getAddon<addon::Transform>()->rotateArround(glm::pi<float>(), glm::vec3(0, 1, 0));
			}

			mWallsGameObjects.emplace(WallKey{ x, y, vertical }, std::move(obj));
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