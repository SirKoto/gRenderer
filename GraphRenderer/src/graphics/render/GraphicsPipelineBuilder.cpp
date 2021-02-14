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
		vk::FrontFace::eCounterClockwise, // front face
		false, 0.0f, 0.0f, 0.0f, // depth bias
		1.0f // line width
	)
{

}

void GraphicsPipelineBuilder::setShaderStages(vk::ShaderModule vertex, vk::ShaderModule fragment)
{
	mVertexModule = vertex;
	mFragmentModule = fragment;
}

void GraphicsPipelineBuilder::setVertexBindingDescriptions(const std::vector<vk::VertexInputBindingDescription>& bindings)
{
	mVertInBindings = bindings;
}

void GraphicsPipelineBuilder::setVertexAttirbuteDescriptions(const std::vector<vk::VertexInputAttributeDescription>& attributes)
{
	mVertInAttributes = attributes;
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

void GraphicsPipelineBuilder::setColorBlendAttachmentAlphaBlending()
{
	mColorBlendAttachment =
		vk::PipelineColorBlendAttachmentState(
			true,									// active blend
			vk::BlendFactor::eSrcAlpha,				// srcColorBlendFactor
			vk::BlendFactor::eOneMinusSrcAlpha,		// dstColorBlendFactor
			vk::BlendOp::eAdd,						// colorBlendOp
			vk::BlendFactor::eOne,					// srcAlphaBlendFactor
			vk::BlendFactor::eZero,					// dstAlphaBlendFactor
			vk::BlendOp::eAdd,						// alphaBlendOp
			vk::ColorComponentFlagBits::eR |		// colorWriteMask
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA);
}

void GraphicsPipelineBuilder::setDepthState(bool testEnable, bool writeEnable, vk::CompareOp compareOp)
{
	mDepthStencilState
		.setDepthTestEnable(testEnable)
		.setDepthWriteEnable(writeEnable)
		.setDepthCompareOp(compareOp);
}

void GraphicsPipelineBuilder::setPipelineLayout(vk::PipelineLayout layout)
{
	mPipLayout = layout;
}

void GraphicsPipelineBuilder::addDynamicState(vk::DynamicState state)
{
	mDynamicStates.push_back(state);
}

vk::Pipeline GraphicsPipelineBuilder::createPipeline(
	vk::Device device,
	vk::RenderPass renderPass,
	uint32_t subpass) const
{

	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
		vk::PipelineShaderStageCreateInfo(
			{}, // flags
			vk::ShaderStageFlagBits::eVertex,
			mVertexModule,
			"main",
			nullptr
		),
		vk::PipelineShaderStageCreateInfo(
			{}, // flags
			vk::ShaderStageFlagBits::eFragment,
			mFragmentModule,
			"main",
			nullptr
		) 
	};

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

	vk::PipelineDynamicStateCreateInfo dynamicState(
		{}, mDynamicStates
	);

	vk::GraphicsPipelineCreateInfo createInfo(
		{}, // flags
		shaderStages, // shader stages
		&vertexInputState,
		&inputAssemblyState,
		nullptr, // tessellation
		&viewportState,
		&mRasterizationState,
		&multisampleState,
		&mDepthStencilState, // depth stencil
		&colorBlendState,
		&dynamicState, // dynamic state
		mPipLayout,
		renderPass,
		subpass
	);

	vk::ResultValue<vk::Pipeline> res = device.createGraphicsPipeline(nullptr, createInfo);
	if (res.result != vk::Result::eSuccess) {
		throw std::runtime_error("Error creating graphics pipeline!!!");
	}

	return res.value;
}

} // namespace vkg
} // namespace gr