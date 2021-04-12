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

    Scene(FrameContext* fc);


    static constexpr const char* s_getClassName() { return "Scene"; }

    virtual void scheduleDestroy(FrameContext* fc) override;

    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override;

    void graphicsUpdate(FrameContext* fc);

private:

    std::unique_ptr<GameObject> mUiCameraGameObj;

    std::set<ResId> mGameObjects;

};

} // namespace gr
