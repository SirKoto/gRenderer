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

	virtual void destroy(GlobalContext* gc) override final;

	virtual void scheduleDestroy(FrameContext* fc) override final;

	virtual void renderImGui(FrameContext* fc) override final;

	static constexpr const char* s_getClassName() { return "Sampler"; }

	operator bool() const { return mSampler; }

protected:

	vk::Sampler mSampler;

	vk::SamplerAddressMode mAddresMode = vk::SamplerAddressMode::eRepeat;
};

} // namespace gr
