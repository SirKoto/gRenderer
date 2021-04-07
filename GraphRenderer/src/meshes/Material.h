#pragma once
#include "IObject.h"

#include <vulkan/vulkan.hpp>

namespace gr
{

class Material :
    public IObject
{
public:

    virtual void scheduleDestroy(FrameContext* fc) override final;
    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override final;

    static constexpr const char* s_getClassName() { return "Material"; }

private:


protected:

}; // class Material

} // namespace gr
