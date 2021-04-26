#pragma once
#include "IAddon.h"

#include <glm/glm.hpp>

namespace gr
{
namespace addon {


class SimplePlayerControl :
    public IAddon
{
public:
    SimplePlayerControl() = default;

    void drawImGuiInspector(FrameContext* fc, GameObject* parent) override;

    void update(FrameContext* fc, GameObject* parent) override;

    const char* getAddonName() override { return SimplePlayerControl::s_getAddonName(); }

    static const char* s_getAddonName() { return "SimplePlayerControl"; }

private:

    float mSpeed = 2.0f;
    float mMouseSpeed = 1.f;

    glm::vec2 mPreMousePos = glm::vec2(0.f);

    // Serialization functions
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(GR_SERIALIZE_NVP_MEMBER(mSpeed));
        ar(GR_SERIALIZE_NVP_MEMBER(mMouseSpeed));
    }

    GR_SERIALIZE_PRIVATE_MEMBERS
};


} // namespace addon
} // namespace gr

GR_SERIALIZE_TYPE(gr::addon::SimplePlayerControl)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::addon::IAddon, gr::addon::SimplePlayerControl)