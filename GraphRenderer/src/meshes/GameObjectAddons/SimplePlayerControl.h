#pragma once
#include "IAddon.h"

#include <glm/glm.hpp>

namespace gr
{
namespace addon {


class SimplePlayerControl :
    public IAddon
{
public:

    SimplePlayerControl(FrameContext* fc) : IAddon(fc) {}

    void drawImGuiInspector(FrameContext* fc, GameObject* parent) override;

    void update(FrameContext* fc, GameObject* parent) override;

    static const char* s_getAddonName() { return "SimplePlayerControl"; }

private:

    float mSpeed = 2.0f;
    float mMouseSpeed = 1.f;

    glm::vec2 mPreMousePos = glm::vec2(0.f);
};


} // namespace addon
} // namespace gr
