#include "GraphicsPipelineBuilder.h"

GraphicsPipelineBuilder::GraphicsPipelineBuilder() : mViewport(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f)
{

}

void GraphicsPipelineBuilder::addVertexBindingDescription(const vk::VertexInputBindingDescription& binding)
{
	mVertInBindings.push_back(binding);
}

void GraphicsPipelineBuilder::addVertexAttributDescription(const vk::VertexInputAttributeDescription& attrib)
{
	mVertInAttributes.push_back(attrib);
}

void GraphicsPipelineBuilder::addVertexBindingDescription(vk::VertexInputBindingDescription&& binding)
{
	mVertInBindings.emplace_back(binding);
}

void GraphicsPipelineBuilder::addVertexAttributDescription(vk::VertexInputAttributeDescription&& attrib)
{
	mVertInAttributes.emplace_back(attrib);
}

void GraphicsPipelineBuilder::setPrimitiveTopology(vk::PrimitiveTopology topology)
{
	mTopology = topology;
}

void GraphicsPipelineBuilder::setViewportSize(const vk::Extent2D& extent)
{
	mViewport.width = static_cast<float>(extent.width);
	mViewport.height = static_cast<float>(extent.height);
	mScissor.extent = extent;
}

vk::Pipeline GraphicsPipelineBuilder::createPipeline() const
{
	vk::PipelineVertexInputStateCreateInfo vertexInputState(
		{}, // flags
		mVertInBindings,
		mVertInAttributes
	);

	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState(
		{},			// flags
		mTopology,	// topology
		false		// primitive restart enable
	);

	return vk::Pipeline();
}

vk::PipelineViewportStateCreateInfo GraphicsPipelineBuilder::buildPipelineViewportState() const
{
	vk::PipelineViewportStateCreateInfo info(
		{},				// flags
		1, &mViewport,
		1, &mScissor
	);

	return info;
}
