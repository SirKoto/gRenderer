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
    Renderable() = default;
    Renderable(FrameContext* fc);

    void drawImGuiInspector(FrameContext* fc, GameObject* parent) override;

    void updateBeforeRender(FrameContext* fc, GameObject* parent, const SceneRenderContext& src) override;

    void destroy(FrameContext* fc) override;

    void setMesh(ResId meshId);

    const char* getAddonName() override { return Renderable::s_getAddonName(); }


    static const char* s_getAddonName() { return "Renderable"; }


private:

    ResId mMesh;

    vkg::Buffer mUbos;
    uint8_t* mUbosGpuPtr = nullptr;
    std::vector<vk::DescriptorSet> mObjectDescriptorSets;

    void createUbos(FrameContext* fc);

    // Serialization functions
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(GR_SERIALIZE_NVP_MEMBER(mMesh));
    }

    GR_SERIALIZE_PRIVATE_MEMBERS
};

} // namespace addon
} // namespace gr


GR_SERIALIZE_TYPE(gr::addon::Renderable)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::addon::IAddon, gr::addon::Renderable)