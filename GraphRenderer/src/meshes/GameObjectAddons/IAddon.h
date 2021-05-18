#pragma once

#include "../../utils/serialization.h"

#include <memory>

namespace gr {

class FrameContext;
class GameObject;
struct SceneRenderContext;

namespace addon {

class IAddon
{
public:
	IAddon() = default;

	IAddon& operator=(const IAddon&) = delete;

	virtual void drawImGuiInspector(FrameContext* fc, GameObject* parent) = 0;

	virtual std::unique_ptr<addon::IAddon> duplicate(FrameContext* fc, const GameObject* parent) const = 0;
	
	virtual void updateBeforeRender(FrameContext* fc, GameObject* parent, const SceneRenderContext& src) {}

	virtual void start(FrameContext* fc) {}

	virtual void update(FrameContext* fc, GameObject* parent) {}

	virtual void destroy(FrameContext* fc) {}

	virtual ~IAddon() {}

	virtual const char* getAddonName() = 0;

	static const char* s_getAddonName() { return "Addon"; }

};


} // namespace addon
} // namespace gr
