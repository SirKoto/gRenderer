#pragma once
#include "IObject.h"

#include <glm/glm.hpp>
#include <map>

#include "../graphics/resources/Buffer.h"

#include "GameObjectAddons/IAddon.h"
#include "GameObjectAddons/Transform.h"

namespace gr
{
class GameObject :
    public IObject
{
public:

    GameObject(FrameContext* fc) : IObject(fc) {}


    virtual void scheduleDestroy(FrameContext* fc) override;
    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override;

    static constexpr const char* s_getClassName() { return "GameObjects"; }

    void setMesh(const ResId& id) { mMesh = id; }

    void graphicsUpdate(FrameContext* fc);

    template <typename Addon>
    Addon* getAddon();
    template <typename Addon>
    const Addon* getAddon() const;

    template <typename Addon>
    bool addAddon(FrameContext* fc);

protected:


    ResId mMesh;

    // addons
    addon::Transform mTransform;

    std::map<const char*, std::unique_ptr<addon::IAddon>> mAddons;

    vkg::Buffer mUbos;
    uint8_t* mUbosGpuPtr = nullptr;
    std::vector<vk::DescriptorSet> mObjectDescriptorSets;

    void createUbos(FrameContext* fc);
};


template<typename Addon>
inline Addon* gr::GameObject::getAddon()
{

    decltype(mAddons)::iterator it = mAddons.find(Addon::s_getAddonName());
    if (it != mAddons.end()) {
        return it->second.get();
    }
    return nullptr;
}

template<typename Addon>
inline const Addon* gr::GameObject::getAddon() const
{
    decltype(mAddons)::const_iterator it = mAddons.find(Addon::s_getAddonName());
    if (it != mAddons.end()) {
        return it->second.get();
    }
    return nullptr;
}

template<>
inline addon::Transform* gr::GameObject::getAddon<addon::Transform>() {
    return &mTransform;
}
template<>
inline const addon::Transform* gr::GameObject::getAddon<addon::Transform>() const {
    return &mTransform;
}

template<typename Addon>
inline bool gr::GameObject::addAddon(FrameContext* fc)
{
    auto it = mAddons.emplace(Addon::s_getAddonName(), new Addon());
    return it.second;
}

template<>
inline bool gr::GameObject::addAddon<addon::Transform>(FrameContext* fc)
{
    return false;
}

} // namespace gr
