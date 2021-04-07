#pragma once
#include "IObject.h"

#include "ResourcesHeader.h"

namespace gr
{

class Pipeline :
    public IObject
{
public:

    virtual void scheduleDestroy(FrameContext* fc) override final;

    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override final;

    static constexpr const char* s_getClassName() { return "Pipeline"; }

private:

    vk::PipelineLayout mPipelineLayout;
    vk::Pipeline mPipeline;

    gr::ResId mMaterialDescriptorLayout;

};

} // namespace gr
