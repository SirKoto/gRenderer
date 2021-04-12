#pragma once
#include "IAddon.h"

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

    float mSpeed = 1.0f;

};


} // namespace addon
} // namespace gr
