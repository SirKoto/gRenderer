#pragma once
#include "IObject.h"

#include <glm/glm.hpp>

#include "../graphics/resources/Buffer.h"

namespace gr
{
class GameObject :
    public IObject
{
public:

    virtual void scheduleDestroy(FrameContext* fc) override;
    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override;

    static constexpr const char* s_getClassName() { return "GameObjects"; }

    void setMesh(const ResId& id) { mMesh = id; }


protected:


    ResId mMesh;

    // Transform
    glm::vec3 mPos = glm::vec3(0.f);
    glm::vec3 mScale = glm::vec3(1.f);
    glm::vec3 mRotation = glm::vec3(0.f);

    vkg::Buffer mUbos;
    void* mUbosGpuPtr = nullptr;

    void createUbos(FrameContext* fc);
};

} // namespace gr
