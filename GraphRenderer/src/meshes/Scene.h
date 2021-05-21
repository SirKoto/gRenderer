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

    Scene();

    static constexpr const char* s_getClassName() { return "Scene"; }

    void scheduleDestroy(FrameContext* fc) override;

    void renderImGui(FrameContext* fc, Gui* gui) override;

    void start(FrameContext* fc) override;

    void graphicsUpdate(FrameContext* fc);

    void logicUpdate(FrameContext* fc);


private:

    std::unique_ptr<GameObject> mUiCameraGameObj;

    std::set<ResId> mGameObjects;

    std::vector<uint64_t> mNumTrisFrameBuff = std::vector<uint64_t>(4, 0);
    double_t mNumTrisFrame = 0.0;

    float mGoalFPSLOD = 60.0f;
    bool mAutomaticLOD = false;


    void lodUpdate(FrameContext* fc);
    void updateNumTrisFrame(FrameContext* fc);

    // Serialization functions
    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(cereal::base_class<IObject>(this));
        archive(GR_SERIALIZE_NVP_MEMBER(mUiCameraGameObj));
        archive(GR_SERIALIZE_NVP_MEMBER(mGameObjects));
        archive(GR_SERIALIZE_NVP_MEMBER(mAutomaticLOD));
    }

    GR_SERIALIZE_PRIVATE_MEMBERS

};

struct SceneRenderContext {
    const addon::Camera* camera;
};

} // namespace gr

GR_SERIALIZE_TYPE(gr::Scene)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::Scene)