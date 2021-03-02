#pragma once
#include "IObject.h"

#include <vulkan/vulkan.hpp>

namespace gr
{

class Material :
    public IObject
{
public:
    // Inherited via IObject
    virtual void destroy(GlobalContext* gc) override final;
    virtual void scheduleDestroy(FrameContext* fc) override final;
    virtual void renderImGui(FrameContext* fc) override final;

    static constexpr const char* s_getClassName() { return "Material"; }

private:


protected:

    vk::PipelineLayout mPipelineLayout;
    vk::Pipeline mPipeline;

}; // class Material

} // namespace gr
