#pragma once

#include "../Context.h"

#include <unordered_map>

namespace gr
{

namespace vkg
{



class DescriptorManager
{
public:
	DescriptorManager() = default;
	DescriptorManager(const Context& context);
	void initialize(const Context& context);

	void destroy(const Context& context);

	void allocateDescriptorSets(
		const Context& context,
		uint32_t num,
		const vk::DescriptorSetLayout layout, 
		vk::DescriptorSet* outLayouts);

	void freeDescriptorSet(vk::DescriptorSet descriptorSet, vk::DescriptorSetLayout layout);

private:
	static constexpr uint32_t MAX_SIZE_RESOURCE = 100;
	vk::DescriptorPool mDescriptorPool;

	std::unordered_multimap<vk::DescriptorSetLayout, vk::DescriptorSet> mDescriptorSetsCache;

};


}
}