#pragma once
#include "IAddon.h"

#include "../ResourcesHeader.h"
#include "../../graphics/resources/Buffer.h"
#include "../../utils/math/BBox.h"

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

    void drawImGuiInspector(FrameContext* fc, GameObject* parent) override;

    void updateBeforeRender(FrameContext* fc, GameObject* parent, const SceneRenderContext& src) override;

    void destroy(FrameContext* fc) override;
    void start(FrameContext* fc) override;

    void setMesh(ResId meshId);

    void setLOD(uint32_t newLod) { mLod = newLod; }
    const uint32_t getLOD() const { return mLod; }
    uint32_t getMaxLOD(FrameContext* fc) const;
    uint32_t getLODDepth(FrameContext* fc, uint32_t lod) const;

    uint32_t getNumTrisToRender(FrameContext* fc, uint32_t lod) const;

    mth::AABBox getBBox(FrameContext* fc) const;

    const char* getAddonName() override { return Renderable::s_getAddonName(); }


    static const char* s_getAddonName() { return "Renderable"; }


private:

    ResId mMesh;

    uint32_t mLod = 0;

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