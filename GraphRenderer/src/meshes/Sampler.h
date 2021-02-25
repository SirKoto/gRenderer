#pragma once

#include "IObject.h"

namespace gr 
{

class Sampler : public IObject
{
public:

	virtual void destroy(GlobalContext* gc) override final;

	virtual void scheduleDestroy(FrameContext* fc) override final;

	virtual void renderImGui(FrameContext* fc) override final;

	static constexpr const char* s_getClassName() { return "Sampler"; }

protected:

};

} // namespace gr
