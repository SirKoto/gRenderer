#include "PipelineBuilder.h"

namespace gr
{
namespace vkg
{


void PipelineBuilder::reserveNumShaderStages(uint32_t num)
{
	mShaderStages.reserve(num);
}

uint32_t PipelineBuilder::addShaderStage(vk::ShaderModule module, vk::ShaderStageFlagBits stage)
{
	
	mShaderStages.emplace_back(
		vk::PipelineShaderStageCreateInfo(
			{}, // flags
			stage, // shader stage
			module, // shader module
			"main", // entry point
			nullptr // Specialization Info
		)
	);

	return static_cast<uint32_t>(mShaderStages.size()) - 1;
}

vk::Pipeline PipelineBuilder::createPipeline() const
{
	assert(false);
	return vk::Pipeline();
}
} // namespace vkg
} // namespace gr