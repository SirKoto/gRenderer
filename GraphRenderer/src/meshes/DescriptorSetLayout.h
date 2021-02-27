#pragma once

#include "IObject.h"

#include "ResourcesHeader.h"

#include <vulkan/vulkan.hpp>
#include <vector>

namespace gr {


class DescriptorSetLayout :
    public IObject
{
public:


	virtual void destroy(GlobalContext * gc) override final;
	virtual void scheduleDestroy(FrameContext * fc) override final;
	virtual void renderImGui(FrameContext * fc) override final;

	static constexpr const char* s_getClassName() { return "DescriptorSetLayout"; }

	operator bool() const { return mDescSetLayout; }

private:
	vk::DescriptorSetLayout mDescSetLayout;

	struct DSL {
		vk::DescriptorSetLayoutBinding binding;
		std::vector<ResId> immutableSamplers;

		DSL();
	};
	std::list<DSL> mBindings;

	void createDescriptorLayout(FrameContext* fc);
};

} // namespace gr
