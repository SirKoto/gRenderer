#pragma once

#include "IObject.h"

#include <vulkan/vulkan.hpp>

namespace gr 
{

class FrameContext;
class GlobalContext;
class Sampler : public IObject
{
public:

	Sampler() = default;


	virtual void scheduleDestroy(FrameContext* fc) override final;

	virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override final;

	static constexpr const char* s_getClassName() { return "Sampler"; }

	operator bool() const { return mSampler; }

	vk::Sampler getVkSampler() const { return mSampler; }


protected:

	vk::Sampler mSampler;

	vk::SamplerAddressMode mAddresMode = vk::SamplerAddressMode::eRepeat;

	// Serialization functions
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(cereal::base_class<IObject>(this));
		//TODO
	}

	GR_SERIALIZE_PRIVATE_MEMBERS

};

} // namespace gr

GR_SERIALIZE_TYPE(gr::Sampler)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::Sampler)