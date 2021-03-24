#include "Scene.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "GameObject.h"
#include "../control/FrameContext.h"

namespace gr
{

void Scene::destroy(GlobalContext* gc)
{
}

void Scene::scheduleDestroy(FrameContext* fc)
{
}

void Scene::renderImGui(FrameContext* fc, GuiFeedback* feedback)
{
	assert(feedback != nullptr);

	if (ImGui::BeginPopupContextWindow()) {
		
		if (ImGui::Button("Add empty")) {

			ResId newId = fc->gc().getDict().allocateObject<GameObject>("Empty GameObject");
			mGameObjects.push_back(newId);
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
				mGameObjects.push_back(id);
			}
			ImGui::EndDragDropTarget();
		}
	}

	decltype(mGameObjects)::iterator it = mGameObjects.begin();
	while (it != mGameObjects.end()) {
		ResId id = *it;

		bool advance = true;

		if (fc->gc().getDict().exists(id)) {
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

} // namespace gr
