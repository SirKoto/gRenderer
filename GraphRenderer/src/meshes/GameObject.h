#pragma once
#include "IObject.h"

namespace gr
{
class GameObject :
    public IObject
{
public:

    virtual void destroy(GlobalContext* gc) override;
    virtual void scheduleDestroy(FrameContext* fc) override;
    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override;

    static constexpr const char* s_getClassName() { return "GameObjects"; }

};

} // namespace gr
