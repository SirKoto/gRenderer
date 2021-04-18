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
	DescriptorSetLayout() = default;
	DescriptorSetLayout(FrameContext* fc) : IObject(fc) {}


	virtual void scheduleDestroy(FrameContext * fc) override final;
	virtual void renderImGui(FrameContext * fc, GuiFeedback* feedback = nullptr) override final;

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

	// Serialization functions
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(cereal::base_class<IObject>(this));
		// TODO
	}

	GR_SERIALIZE_PRIVATE_MEMBERS

};

} // namespace gr

GR_SERIALIZE_TYPE(gr::DescriptorSetLayout)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::DescriptorSetLayout)