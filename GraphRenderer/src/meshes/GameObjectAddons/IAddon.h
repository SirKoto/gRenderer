#pragma once

namespace gr {

class FrameContext;
class GameObject;
struct SceneRenderContext;

namespace addon {

class IAddon
{
public:
	
	IAddon(FrameContext* fc) {}

	virtual void drawImGuiInspector(FrameContext* fc, GameObject* parent) = 0;
	
	virtual void updateBeforeRender(FrameContext* fc, GameObject* parent, const SceneRenderContext& src) {};

	virtual void update(FrameContext* fc, GameObject* parent) {};

	virtual void destroy(FrameContext* fc) {}

	virtual ~IAddon() {}

	static const char* s_getAddonName() { return "Addon"; }

};


} // namespace addon
} // namespace gr
