#pragma once

#include "IObject.h"
#include "GameObject.h"
#include "GameObjectAddons/Camera.h"
#include "SceneControl/VisibilityGrid.h"


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

    double_t getTrianglesPerFrame() const { return mNumTrisFrame; }


private:

    std::unique_ptr<GameObject> mUiCameraGameObj;
    std::unique_ptr<VisibilityGrid> mVisibilityGrid;

    std::set<ResId> mGameObjects;

    std::vector<uint64_t> mNumTrisFrameBuff = std::vector<uint64_t>(4, 0);
    double_t mNumTrisFrame = 0.0;

    float mGoalFPSLOD = 60.0f;
    bool mAutomaticLOD = false;
    bool mCellVisibility = false;
    bool mVisibilityGridMenuOpen = false;

    void lodUpdate(FrameContext* fc, const std::set<ResId>& gameObjectsToRender);
    void updateNumTrisFrame(FrameContext* fc, const std::set<ResId>& renderedObjects);

    // Serialization functions
    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(cereal::base_class<IObject>(this));
        archive(GR_SERIALIZE_NVP_MEMBER(mUiCameraGameObj));
        archive(GR_SERIALIZE_NVP_MEMBER(mGameObjects));
        archive(GR_SERIALIZE_NVP_MEMBER(mAutomaticLOD));
        archive(GR_SERIALIZE_NVP_MEMBER(mCellVisibility));
        archive(GR_SERIALIZE_NVP_MEMBER(mVisibilityGrid));
    }

    GR_SERIALIZE_PRIVATE_MEMBERS

};

struct SceneRenderContext {
    const addon::Camera* camera;
};

} // namespace gr

GR_SERIALIZE_TYPE(gr::Scene)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::Scene)