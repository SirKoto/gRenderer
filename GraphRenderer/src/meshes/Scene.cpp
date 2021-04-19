#include "Scene.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "GameObject.h"
#include "GameObjectAddons/Camera.h"
#include "GameObjectAddons/SimplePlayerControl.h"
#include "../control/FrameContext.h"
#include "../utils/grjob.h"


namespace gr
{

void Scene::scheduleDestroy(FrameContext* fc)
{
	if (mUiCameraGameObj) {
		mUiCameraGameObj->scheduleDestroy(fc);
	}
}

void Scene::renderImGui(FrameContext* fc, GuiFeedback* feedback)
{
	assert(feedback != nullptr);

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


	if (ImGui::TreeNode("Scene Camera")) {
		mUiCameraGameObj->renderImGui(fc, nullptr);
		ImGui::TreePop();
	}
	ImGui::Separator();
	// Other gameobjects
	decltype(mGameObjects)::iterator it = mGameObjects.begin();
	while (it != mGameObjects.end()) {
		ResId id = *it;

		bool advance = true;

		if (fc->gc().getDict().exists(id)) {

			ImGui::PushID((void*)std::hash<ResId>()(id));
			if (ImGui::Button(fc->gc().getDict().getName(id).c_str())) {
				feedback->selectResource = id;
			}

			// context on button
			if (ImGui::BeginPopupContextItem()) {

				if (ImGui::Button("Remove")) {
					it = mGameObjects.erase(it);
					advance = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

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

void Scene::start(FrameContext* fc)
{
	mUiCameraGameObj = std::make_unique<GameObject>();
	mUiCameraGameObj->start(fc);
	bool res = mUiCameraGameObj->addAddon<addon::Camera>(fc);
	res &= mUiCameraGameObj->addAddon<addon::SimplePlayerControl>(fc);
	assert(res);
}

} // namespace gr
