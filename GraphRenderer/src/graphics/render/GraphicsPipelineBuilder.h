#pragma once

#include "PipelineBuilder.h"

namespace gr
{
namespace vkg
{

class GraphicsPipelineBuilder : public PipelineBuilder
{
public:

	GraphicsPipelineBuilder();

	// Pipeline shader stages
	void setShaderStages(vk::ShaderModule vertex, vk::ShaderModule fragment);

	// Pipeline Vertex Input State
	void setVertexBindingDescriptions(const std::vector<vk::VertexInputBindingDescription>& bindings);
	void setVertexAttirbuteDescriptions(const std::vector<vk::VertexInputAttributeDescription>& attributes);
	void addVertexBindingDescription(const vk::VertexInputBindingDescription& binding);
	void addVertexAttributDescription(const vk::VertexInputAttributeDescription& attrib);
	void addVertexBindingDescription(vk::VertexInputBindingDescription&& binding);
	void addVertexAttributDescription(vk::VertexInputAttributeDescription&& attrib);

	// PipelineInputAssemblyState
	void setPrimitiveTopology(vk::PrimitiveTopology topology);
	void setPolygonMode(vk::PolygonMode polyMode);

	// ViewportStateCreateInfo
	void setViewportSize(const vk::Extent2D& extent);

	// RasterizationState
	// by default
	// {}, false, false,// flags, depthClamp, rasterizerDiscard
	// vk::PolygonMode::eFill, // pollygon mode
	//	vk::CullModeFlagBits::eBack, // cull back faces
	//	vk::FrontFace::eCounterClockwise // front face

	// Multisampling
	void setMultisampleCount(vk::SampleCountFlagBits sampleCount);

	// ColorBlendState (only can have one attachment)
	// RGBA, no blending
	void setColorBlendAttachmentStd();

	void setColorBlendAttachmentAlphaBlending();

	// DepthConfiguration
	void setDepthState(
		bool testEnable,
		bool writeEnable,
		vk::CompareOp compareOp);

	// Pipeline Layout
	void setPipelineLayout(vk::PipelineLayout layout);

	// Dynamic State
	void addDynamicState(vk::DynamicState state);

	void setFrontFace(vk::FrontFace frontFace) { mRasterizationState.setFrontFace(frontFace); }

	void setCulling(vk::CullModeFlagBits culling) { mRasterizationState.setCullMode(culling); }

	vk::Pipeline createPipeline(vk::Device device, vk::RenderPass renderPass, uint32_t subpass) const;

protected:

	// Shader modules for stages
	vk::ShaderModule mVertexModule, mFragmentModule;

	// Pipeline Vertex Input State
	std::vector<vk::VertexInputBindingDescription> mVertInBindings;
	std::vector<vk::VertexInputAttributeDescription> mVertInAttributes;

	// Dynamic States
	std::vector<vk::DynamicState> mDynamicStates;

	// PipelineInputAssemblyState
	vk::PrimitiveTopology mTopology = vk::PrimitiveTopology::eTriangleList;

	// ViewportStateCreateInfo
	vk::Viewport mViewport;
	vk::Rect2D mScissor;

	// RasterizationState
	vk::PipelineRasterizationStateCreateInfo mRasterizationState;

	// DepthStencilState. Default all false
	vk::PipelineDepthStencilStateCreateInfo mDepthStencilState;

	// Multisampling
	vk::SampleCountFlagBits mMultisampleCount = vk::SampleCountFlagBits::e1;

	// ColorBlendState
	vk::PipelineColorBlendAttachmentState mColorBlendAttachment;

	// Pipeline Layout
	vk::PipelineLayout mPipLayout;
};


} // namespace vkg
} // namespace gr