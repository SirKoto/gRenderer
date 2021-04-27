#include "RenderSubmitter.h"

namespace gr
{
namespace vkg
{



bool RenderSubmitter::MaterialKey::operator==(const MaterialKey& o) const
{

    return this->pipeline == o.pipeline &&
        this->materialDescriptorSet == o.materialDescriptorSet;
}

void RenderSubmitter::pushPredefinedDraw(const DrawData& drawData)
{
    assert(mDefaultMaterial.pipeline);

    mMaterialRenderList.at(mDefaultMaterial).renderList.push_back(drawData);
}

void RenderSubmitter::setDefaultMaterial(
    const vk::Pipeline pipeline,
    const vk::PipelineLayout pipLayout,
    const vk::DescriptorSet descriptorSet)
{
    MaterialKey key{ pipeline, descriptorSet };
    Material mat{ pipLayout };

    if (mMaterialRenderList.count(mDefaultMaterial)) {
        mMaterialRenderList.erase(mDefaultMaterial);
    }

    mDefaultMaterial = key;



    mMaterialRenderList.insert({ key, mat });
}

void RenderSubmitter::setSceneDescriptorSet(const vk::DescriptorSet descriptor)
{
    mSceneDescriptorSet = descriptor;
}

void RenderSubmitter::flushDraws(vk::CommandBuffer cmd)
{
    assert(cmd);

    if (!mSceneDescriptorSet) {
        return;
    }

    bool firstBindDescriptors = true;

    for (auto& material : mMaterialRenderList) {

        if (firstBindDescriptors) {
            firstBindDescriptors = false;
            // bind to 0
            cmd.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,   // bind point
                material.second.pipelineLayout,     // pipeline layout
                0, 1,                               // set and number of sets
                &mSceneDescriptorSet,// desc set
                0, nullptr                          // no dynamic offsets
            );
        }

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, material.first.pipeline);
        if (material.first.materialDescriptorSet) {
            // bind to 1
            cmd.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,   // bind point
                material.second.pipelineLayout,     // pipeline layout
                1, 1,                               // set and number of sets
                &material.first.materialDescriptorSet,// desc set
                0, nullptr                          // no dynamic offsets
                );
        }

        for (const DrawData& dd : material.second.renderList) {
            if (dd.objectDescriptorSet) {
                // bind to 2
                cmd.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,   // bind point
                    material.second.pipelineLayout,     // pipeline layout
                    2, 1,                               // set and number of sets
                    &dd.objectDescriptorSet,// desc set
                    0, nullptr                          // no dynamic offsets
                );
            }

            // bind to 0
            cmd.bindVertexBuffers(0, 1, &dd.vertexBuffer, &dd.vertexBufferOffset);
            cmd.bindIndexBuffer(dd.indexBuffer, 0, vk::IndexType::eUint32);

            cmd.drawIndexed(dd.numIndices, 1, dd.firstIndex, 0, 0);
        }

        material.second.renderList.clear();

    }

}

}
}
