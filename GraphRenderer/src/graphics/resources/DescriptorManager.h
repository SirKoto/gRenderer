#pragma once

#include <vulkan/vulkan.hpp>

#include <unordered_map>

namespace gr
{

namespace vkg
{


class RenderContext;
class DescriptorManager
{
public:
	DescriptorManager() = default;
	DescriptorManager(const RenderContext& context);


	DescriptorManager& operator=(const DescriptorManager& o) = delete;

	void initialize(const RenderContext& context);

	void destroy(const RenderContext& context);

	void allocateDescriptorSets(
		const RenderContext& context,
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