#include "GraphicsPipelineBuilder.h"

namespace gr
{
namespace vkg
{

GraphicsPipelineBuilder::GraphicsPipelineBuilder() : PipelineBuilder(),
	mViewport(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), // init viewport with everything zeroes
	mRasterizationState( // Init rasterization state
		{}, false, false,// flags, depthClamp, rasterizerDiscard
		vk::PolygonMode::eFill, // pollygon mode
		vk::CullModeFlagBits::eBack, // cull back faces
		vk::FrontFace::eCounterClockwise // front face
	)
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

void GraphicsPipelineBuilder::setMultisampleCount(vk::SampleCountFlagBits sampleCount)
{
	mMultisampleCount = sampleCount;
}

void GraphicsPipelineBuilder::setColorBlendAttachmentStd()
{
	 mColorBlendAttachment = 
		vk::PipelineColorBlendAttachmentState(
			false,									// active blend
			vk::BlendFactor::eZero,					// srcColorBlendFactor
			vk::BlendFactor::eZero,					// dstColorBlendFactor
			vk::BlendOp::eAdd,						// colorBlendOp
			vk::BlendFactor::eZero,					// srcAlphaBlendFactor
			vk::BlendFactor::eZero,					// dstAlphaBlendFactor
			vk::BlendOp::eAdd,						// alphaBlendOp
				vk::ColorComponentFlagBits::eR |	// colorWriteMask
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA);
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

	vk::PipelineViewportStateCreateInfo viewportState(
		{},				// flags
		1, &mViewport,
		1, &mScissor
	);

	vk::PipelineMultisampleStateCreateInfo multisampleState(
		{},
		mMultisampleCount
	);

	vk::PipelineColorBlendStateCreateInfo colorBlendState(
		{},			// flags
		false,		// logicOpEnable
		vk::LogicOp::eClear, // logicOp
		1, &mColorBlendAttachment,	// color attachment
		{}			// blendConstants
	);

	return vk::Pipeline();
}

} // namespace vkg
} // namespace gr