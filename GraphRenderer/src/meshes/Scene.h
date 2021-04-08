#pragma once

#include "IObject.h"

#include <set>

namespace gr
{

class Scene :
    public IObject
{
public:

    static constexpr const char* s_getClassName() { return "Scene"; }

    virtual void scheduleDestroy(FrameContext* fc) override;

    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override;

    void graphicsUpdate(FrameContext* fc);

private:

    std::set<ResId> mGameObjects;

};

} // namespace gr
