#pragma once

#include "IObject.h"
#include "GameObject.h"
#include "GameObjectAddons/Camera.h"

#include <set>



namespace gr
{

class Scene :
    public IObject
{
public:

    Scene() = default;

    static constexpr const char* s_getClassName() { return "Scene"; }

    void scheduleDestroy(FrameContext* fc) override;

    void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override;

    void start(FrameContext* fc) override;

    void graphicsUpdate(FrameContext* fc);

    void logicUpdate(FrameContext* fc);


private:

    std::unique_ptr<GameObject> mUiCameraGameObj;

    std::set<ResId> mGameObjects;

    // Serialization functions
    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(cereal::base_class<IObject>(this));
        archive(mUiCameraGameObj);
        archive(mGameObjects);
    }

    GR_SERIALIZE_PRIVATE_MEMBERS

};

struct SceneRenderContext {
    const addon::Camera* camera;
};

} // namespace gr

GR_SERIALIZE_TYPE(gr::Scene)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::Scene)