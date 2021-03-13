#include "Scene.h"

#include <imgui/imgui.h>
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
	decltype(mGameObjects)::iterator it = mGameObjects.begin();
	while (it != mGameObjects.end()) {
		ResId id = *it;

		if (fc->gc().getDict().exists(id)) {
			if (ImGui::Button(fc->gc().getDict().getName(id).c_str())) {
				feedback->selectResource = id;
			}

			++it;
		}
		else {
			it = mGameObjects.erase(it);
		}
	}
	
}

} // namespace gr
