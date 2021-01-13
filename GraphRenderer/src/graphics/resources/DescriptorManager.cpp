#include "DescriptorManager.h"

namespace gr
{
namespace vkg
{



DescriptorManager::DescriptorManager(const Context& context)
{
	initialize(context);
}

void DescriptorManager::initialize(const Context& context)
{
	// If it is already initialized, destroy
	if (mDescriptorPool) {
		destroy(context);
	}

	typedef vk::DescriptorPoolSize DPS;
	std::array< DPS, 2> poolSizes =
	{
		DPS{vk::DescriptorType::eUniformBuffer, MAX_SIZE_RESOURCE},
		DPS{vk::DescriptorType::eCombinedImageSampler, MAX_SIZE_RESOURCE}
	};

	

	vk::DescriptorPoolCreateInfo createInfo(
		{},
		2 * MAX_SIZE_RESOURCE, // max sets
		poolSizes
	);

	mDescriptorPool = context.getDevice().createDescriptorPool(createInfo);
}

void DescriptorManager::destroy(const Context& context)
{
	context.getDevice().destroyDescriptorPool(mDescriptorPool);
	mDescriptorSetsCache.clear();
	mDescriptorPool = nullptr;
}

void DescriptorManager::allocateDescriptorSets(
	const Context& context,
	uint32_t num, 
	const vk::DescriptorSetLayout layout,
	vk::DescriptorSet* outLayouts)
{
	// First check if there exist cached descriptor sets of the layout
	typedef std::pair<decltype(mDescriptorSetsCache)::const_iterator, decltype(mDescriptorSetsCache)::const_iterator> Range;
	Range range = mDescriptorSetsCache.equal_range(layout);
	if (range.first != range.second) {
		uint32_t numCached = 0;
		decltype(mDescriptorSetsCache)::const_iterator it = range.first;
		while (it != range.second && numCached < num) {
			outLayouts[numCached] = it->second;
			// advance and delete entry (post-increment)
			mDescriptorSetsCache.erase(it++);
			numCached += 1;
		}

		// move pointer and update num
		outLayouts += numCached;
		num -= numCached;
	}

	// if no descriptor sets to create... delete
	if (num == 0) {
		return;
	}

	std::vector<vk::DescriptorSetLayout> layouts(num, layout);
	vk::DescriptorSetAllocateInfo allocInfo(
		mDescriptorPool,
		num, layouts.data()
	);

	vk::Result res = context.getDevice().allocateDescriptorSets(&allocInfo, outLayouts);
	if (res != vk::Result::eSuccess) {
		throw std::runtime_error("Error: Cannot allocate descriptor set!!");
	}
}

void DescriptorManager::freeDescriptorSet(vk::DescriptorSet descriptorSet, vk::DescriptorSetLayout layout)
{
	mDescriptorSetsCache.emplace(layout, descriptorSet);
}

}
}