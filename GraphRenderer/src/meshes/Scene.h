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

    virtual void destroy(GlobalContext* gc) override;

    virtual void scheduleDestroy(FrameContext* fc) override;

    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override;

private:

    std::set<ResId> mGameObjects;

};

} // namespace gr
