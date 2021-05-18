#include "Scene.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "GameObject.h"
#include "GameObjectAddons/Camera.h"
#include "GameObjectAddons/SimplePlayerControl.h"
#include "GameObjectAddons/Renderable.h"
#include "../control/FrameContext.h"
#include "../utils/grjob.h"
#include "../gui/Gui.h"

#include <queue>


namespace gr
{
Scene::Scene()
{
	mUiCameraGameObj = std::make_unique<GameObject>();
}
void Scene::scheduleDestroy(FrameContext* fc)
{
	if (mUiCameraGameObj) {
		mUiCameraGameObj->scheduleDestroy(fc);
	}
}

void Scene::renderImGui(FrameContext* fc, Gui* gui)
{
	

	if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
		
		if (ImGui::Button("Add empty")) {

			ResId newId = fc->gc().getDict().allocateObject<GameObject>(fc, "Empty GameObject");
			mGameObjects.insert(newId);
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// Drop target to drop gameobjects
	{
		ImVec2 min = ImGui::GetWindowPos();
		ImVec2 max = ImGui::GetWindowSize();
		max.x += min.x;
		max.y += min.y;

		if (ImGui::BeginDragDropTargetCustom(
			ImRect{ min, max },
			ImGui::GetCurrentWindow()->ID)) {
			if (const ImGuiPayload* payload =
				ImGui::AcceptDragDropPayload(GameObject::s_getClassName()))
			{
				assert(payload->DataSize == sizeof(ResId));
				ResId id = *reinterpret_cast<ResId*>(payload->Data);
				mGameObjects.insert(id);
			}
			ImGui::EndDragDropTarget();
		}
	}

	if (ImGui::TreeNode("Scene Configuration")) {
		if (ImGui::TreeNode("Scene Camera")) {
			mUiCameraGameObj->renderImGui(fc, nullptr);
			ImGui::TreePop();
		}

		ImGui::Checkbox("Automatic LOD", &mAutomaticLOD);
		int32_t step = 1;
		ImGui::InputFloat("LOD goal FPS", &mGoalFPSLOD, 1.0f, 10.0f);

		ImGui::TreePop();
	}
	ImGui::Separator();
	// Other gameobjects
	decltype(mGameObjects)::iterator it = mGameObjects.begin();
	while (it != mGameObjects.end()) {
		ResId id = *it;

		bool advance = true;

		if (fc->gc().getDict().exists(id)) {

			uint64_t hash = std::hash<ResId>()(id);
			ImGui::PushID((void*)hash);
			std::string buttonName = fc->gc().getDict().getName(id) + "###" + std::to_string(hash);
			if (ImGui::Button(buttonName.c_str())) {
				gui->selectResourceInspector( id );
			}

			// context on button
			if (ImGui::BeginPopupContextItem()) {

				if (ImGui::Button("Remove from scene")) {
					it = mGameObjects.erase(it);
					advance = false;
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::Button("Duplicate")) {
					GameObject* obj = nullptr;
					fc->gc().getDict().get(*it, &obj);
					GameObject* newObj = nullptr;
					std::string name = fc->gc().getDict().getName(*it);
					size_t found = name.find_last_of("_");
					if (found != std::string::npos) {
						name = name.substr(0, found);
					}
					ResId newId = fc->gc().getDict().allocateObject<GameObject>(fc, name, &newObj);
					obj->duplicateTo(fc, newObj);
					mGameObjects.insert(newId);
					ImGui::CloseCurrentPopup();
				}

				ImGui::Separator();

				ImGui::EndPopup();
			}

			gui->appendRenamePopupItem(fc, fc->gc().getDict().getName(id));

			ImGui::PopID();
		}
		else {
			it = mGameObjects.erase(it);
			advance = false;
		}

		if (advance) {
			++it;
		}
	}
	
}


void Scene::graphicsUpdate(FrameContext* fc)
{

	this->lodUpdate(fc);

	std::vector<grjob::Job> jobs;
	jobs.reserve(mGameObjects.size() + 1);

	const SceneRenderContext src = { mUiCameraGameObj.get()->getAddon<addon::Camera>() };

	if (mUiCameraGameObj) {
		jobs.push_back(grjob::Job(&GameObject::graphicsUpdate, mUiCameraGameObj.get(), fc, src));
	}
	
	for (ResId id : mGameObjects) {
		GameObject* obj;
		fc->gc().getDict().get(id, &obj);

		jobs.push_back(grjob::Job(&GameObject::graphicsUpdate, obj, fc, src));
	}


	grjob::Counter* c = nullptr;
	grjob::runJobBatch(grjob::Priority::eMid, jobs.data(), (uint32_t)jobs.size(), &c);
	grjob::waitForCounterAndFree(c, 0);
}

void Scene::logicUpdate(FrameContext* fc)
{
	std::vector<grjob::Job> jobs;
	jobs.reserve(mGameObjects.size() + 1);

	if (mUiCameraGameObj) {
		jobs.push_back(grjob::Job(&GameObject::logicUpdate, mUiCameraGameObj.get(), fc));
	}

	for (ResId id : mGameObjects) {
		GameObject* obj;
		fc->gc().getDict().get(id, &obj);

		jobs.push_back(grjob::Job(&GameObject::logicUpdate, obj, fc));
	}


	grjob::Counter* c = nullptr;
	grjob::runJobBatch(grjob::Priority::eMid, jobs.data(), (uint32_t)jobs.size(), &c);
	grjob::waitForCounterAndFree(c, 0);
}

void Scene::lodUpdate(FrameContext* fc)
{
	// If not automatic LOD, downgrade the LOD if not exists
	if (!mAutomaticLOD) {
		for (ResId id : mGameObjects) {
			GameObject* obj;
			fc->gc().getDict().get(id, &obj);
			addon::Renderable* rend = obj->getAddon<addon::Renderable>();
			if (rend != nullptr) {
				if (rend->getLOD() > rend->getMaxLOD(fc)) {
					rend->setLOD(rend->getMaxLOD(fc));
				}
			}

		} // end for

		return;
	}

	// If automatic LOD, but very high framerrate, just use the high lod models
	if (mGoalFPSLOD >= 1.5 * (1.0 / fc->dt())) {
		for (ResId id : mGameObjects) {
			GameObject* obj;
			fc->gc().getDict().get(id, &obj);
			addon::Renderable* rend = obj->getAddon<addon::Renderable>();
			if (rend != nullptr) {
				rend->setLOD(0);
			}

		} // end for

		return;
	}

	// If automatic LOD, use heuristic to maximize
	struct LodData {
		addon::Renderable* pRenderable;
		float_t diagOverDist;
	};
	std::vector<LodData> renderables;
	renderables.reserve(mGameObjects.size());
	// compute actual number of triangles to render
	uint64_t numTris = 0;
	for (ResId id : mGameObjects) {
		GameObject* obj;
		fc->gc().getDict().get(id, &obj);
		addon::Renderable* rend = obj->getAddon<addon::Renderable>();
		if (rend != nullptr) {
			numTris += rend->getNumTrisToRender(fc, rend->getLOD());
			
			// Compute the division (diagonal / distance)
			const mth::AABBox gameObjectBB = obj->getRenderBB(fc);
			const float sqDiagonal = glm::dot(gameObjectBB.getSize(), gameObjectBB.getSize());
			const glm::vec3 center = gameObjectBB.getMin() + gameObjectBB.getSize() * 0.5f;
			const glm::vec3 viewToObj = center - obj->getAddon<addon::Transform>()->getPos();
			const float sqDistance = glm::dot(viewToObj, viewToObj);

			renderables.push_back({ rend, std::sqrtf(sqDiagonal / sqDistance) });
		}

	} // end for

	const double_t TPS = static_cast<double_t>(numTris) / fc->dt();
	const uint64_t maxCost = static_cast<uint32_t>(TPS / mGoalFPSLOD);
	uint64_t cost = 0;
	typedef std::pair<uint32_t, float_t> QueueVal;
	auto comparator = [](QueueVal a, QueueVal b) -> bool {
		return a.second > b.second;
	};
	std::priority_queue<QueueVal, std::vector<QueueVal>, decltype(comparator)> queueLods(comparator);
	auto getDeltaBenefit = [&renderables, fc](uint32_t i, uint32_t lod) -> float_t {
		const uint32_t actualDepth = renderables[i].pRenderable->getLODDepth(fc, lod);
		const uint32_t nextDepth = (lod == 1) ? std::min(11u, 2 * actualDepth) : renderables[i].pRenderable->getLODDepth(fc, lod - 1);
		const uint32_t deltaDepth = nextDepth - actualDepth;
		float_t num = (float_t)((1 << deltaDepth) - 1);
		float_t denom = (float_t)(1 << nextDepth);
		return renderables[i].diagOverDist * num / denom;
	};

	for (uint32_t i = 0; i < (uint32_t)renderables.size(); ++i) {
		// Only add those that can be improved
		const uint32_t maxLod = renderables[i].pRenderable->getMaxLOD(fc);
		if (maxLod > 0) {
			// set to lowest definition LOD
			renderables[i].pRenderable->setLOD(maxLod);
			// Delta triangles
			const float_t deltaCost = (float_t) (renderables[i].pRenderable->getNumTrisToRender(fc, maxLod - 1) - renderables[i].pRenderable->getNumTrisToRender(fc, maxLod));
			// compute value
			float_t deltaBenefit = getDeltaBenefit(i, maxLod);
			queueLods.push({ i, deltaBenefit / deltaCost });
			cost += renderables[i].pRenderable->getNumTrisToRender(fc, maxLod);
		}
		else { // else set to lod 0, and add its triangles to the cost
			renderables[i].pRenderable->setLOD(0);
			cost += renderables[i].pRenderable->getNumTrisToRender(fc, 0);
		}
	}

	while (!queueLods.empty()) {
		// get and pop best improvement
		const uint32_t top = queueLods.top().first; queueLods.pop();
		const uint32_t lod = renderables[top].pRenderable->getLOD();
		// Check if improvement can be applied. If not, ignore this case
		const uint32_t deltaCost = renderables[top].pRenderable->getNumTrisToRender(fc, lod - 1) - renderables[top].pRenderable->getNumTrisToRender(fc, lod);
		if (cost + deltaCost > maxCost) {
			continue;
		}

		// Upgrade LOD
		renderables[top].pRenderable->setLOD(lod - 1);
		cost += deltaCost;

		// if can keep improving.. store into the priority queue
		if (lod > 1) {
			// Delta triangles
			const float_t newDeltaCost = (float_t)(renderables[top].pRenderable->getNumTrisToRender(fc, lod - 2) - renderables[top].pRenderable->getNumTrisToRender(fc, lod - 1));
			// compute value
			float_t deltaBenefit = getDeltaBenefit(top, lod - 1);
			queueLods.push({ top, deltaBenefit / newDeltaCost });
		}
	}

}

void Scene::start(FrameContext* fc)
{
	mUiCameraGameObj->start(fc);
	mUiCameraGameObj->addAddon<addon::Camera>(fc);
	mUiCameraGameObj->addAddon<addon::SimplePlayerControl>(fc);
}

} // namespace gr
