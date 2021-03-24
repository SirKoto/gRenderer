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

    virtual void destroy(GlobalContext* gc) override;
    virtual void scheduleDestroy(FrameContext* fc) override;
    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override;

    static constexpr const char* s_getClassName() { return "GameObjects"; }



protected:

    glm::vec3 mPos = glm::vec3(0.f);
    glm::vec3 mScale = glm::vec3(1.f);
    glm::vec3 mRotation = glm::vec3(0.f);

    ResId mMesh;

    vkg::Buffer mUbos;

    void createUbos(FrameContext* fc);
};

} // namespace gr
