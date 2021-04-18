#pragma once
#include "IObject.h"

#include <vulkan/vulkan.hpp>

namespace gr
{

class Material :
    public IObject
{
public:

	Material() = default;
    Material(FrameContext* fc) : IObject(fc) {}


    virtual void scheduleDestroy(FrameContext* fc) override final;
    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override final;

    static constexpr const char* s_getClassName() { return "Material"; }

private:


	// Serialization functions
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(cereal::base_class<IObject>(this));
		//TODO
	}

	GR_SERIALIZE_PRIVATE_MEMBERS

}; // class Material

} // namespace gr

GR_SERIALIZE_TYPE(gr::Material)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::Material)