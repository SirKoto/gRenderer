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
    GameObject() = default;
    GameObject(FrameContext* fc);


    virtual void scheduleDestroy(FrameContext* fc) override;
    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override;

    static constexpr const char* s_getClassName() { return "GameObjects"; }

    void graphicsUpdate(FrameContext* fc, const SceneRenderContext& src);

    void logicUpdate(FrameContext* fc);

    template <typename Addon>
    Addon* getAddon();
    template <typename Addon>
    const Addon* getAddon() const;

    template <typename Addon>
    bool addAddon(FrameContext* fc);

protected:

    // addons
    addon::Transform mTransform;

    std::map<const char*, std::unique_ptr<addon::IAddon>> mAddons;

    // Serialization functions

    template<class Archive>
    void save(Archive& ar) const
    {
        ar(cereal::base_class<IObject>(this));
        ar(GR_SERIALIZE_NVP_MEMBER(mTransform));

        uint32_t numAddons = static_cast<uint32_t>(mAddons.size());
        ar(numAddons);
        for (decltype(mAddons)::const_iterator it = mAddons.begin(); it != mAddons.end(); ++it) {
            ar(it->second);
        }
    }

    template<class Archive>
    void load(Archive& ar)
    {
        ar(cereal::base_class<IObject>(this));
        ar(GR_SERIALIZE_NVP_MEMBER(mTransform));

        uint32_t numAddons;
        ar(numAddons);
        decltype(mAddons)::mapped_type ty;
        while (numAddons-- > 0) {
            ar(ty);
            const char* name = ty->getAddonName();
            mAddons.emplace(name, std::move(ty));
        }

    }

    GR_SERIALIZE_PRIVATE_MEMBERS
};

template<typename Addon>
inline Addon* gr::GameObject::getAddon()
{

    decltype(mAddons)::iterator it = mAddons.find(Addon::s_getAddonName());
    if (it != mAddons.end()) {
        return reinterpret_cast<Addon*>(it->second.get());
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
    auto it = mAddons.emplace(Addon::s_getAddonName(), new Addon(fc));
    return it.second;
}

template<>
inline bool gr::GameObject::addAddon<addon::Transform>(FrameContext* fc)
{
    return false;
}

} // namespace gr

CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(gr::GameObject, cereal::specialization::member_load_save)
GR_SERIALIZE_TYPE(gr::GameObject)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::GameObject)