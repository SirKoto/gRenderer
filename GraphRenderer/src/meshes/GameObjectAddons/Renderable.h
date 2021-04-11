#pragma once
#include "IAddon.h"

#include "../ResourcesHeader.h"
#include "../../graphics/resources/Buffer.h"

#include <vulkan/vulkan.hpp>

namespace gr
{
namespace addon
{

class Renderable :
    public IAddon
{
public:
    Renderable(FrameContext* fc);

    void drawImGuiInspector(FrameContext* fc, GameObject* parent) override;

    void updateBeforeRender(FrameContext* fc, GameObject* parent) override;

    void destroy(FrameContext* fc) override;

    void setMesh(ResId meshId);


    static const char* s_getAddonName() { return "Renderable"; }


private:

    ResId mMesh;

    vkg::Buffer mUbos;
    uint8_t* mUbosGpuPtr = nullptr;
    std::vector<vk::DescriptorSet> mObjectDescriptorSets;

    void createUbos(FrameContext* fc);
};

} // namespace addon
} // namespace gr

