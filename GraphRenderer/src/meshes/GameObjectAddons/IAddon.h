#pragma once

namespace gr {

class FrameContext;
class GameObject;

namespace addon {

class IAddon
{
public:
	
	virtual void drawImGuiInspector(FrameContext* fc, GameObject* parent) = 0;
	virtual void preRenderUpdate(FrameContext* fc, GameObject* parent) {}
	virtual void destroy(FrameContext* fc) {}

	virtual ~IAddon() {}

	static const char* s_getAddonName() { return "Addon"; }

};


} // namespace addon
} // namespace gr
