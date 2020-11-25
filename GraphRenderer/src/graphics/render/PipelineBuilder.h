#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

class PipelineBuilder
{
public:

	void reserveNumShaderStages(uint32_t num);
	uint32_t addShaderStage(vk::ShaderModule module, vk::ShaderStageFlagBits stage);

	// This should not be called
	vk::Pipeline createPipeline() const;

protected:

	std::vector<vk::PipelineShaderStageCreateInfo> mShaderStages;

};

