#pragma once

#include "PipelineBuilder.h"

class GraphicsPipelineBuilder : public PipelineBuilder
{
public:

	GraphicsPipelineBuilder();

	// Pipeline Vertex Input State
	void addVertexBindingDescription(const vk::VertexInputBindingDescription& binding);
	void addVertexAttributDescription(const vk::VertexInputAttributeDescription& attrib);
	void addVertexBindingDescription(vk::VertexInputBindingDescription&& binding);
	void addVertexAttributDescription(vk::VertexInputAttributeDescription&& attrib);

	// PipelineInputAssemblyState
	void setPrimitiveTopology(vk::PrimitiveTopology topology);

	// ViewportStateCreateInfo
	void setViewportSize(const vk::Extent2D& extent);

	vk::Pipeline createPipeline() const;

protected:

	// Pipeline Vertex Input State
	std::vector<vk::VertexInputBindingDescription> mVertInBindings;
	std::vector<vk::VertexInputAttributeDescription> mVertInAttributes;

	// PipelineInputAssemblyState
	vk::PrimitiveTopology mTopology = vk::PrimitiveTopology::eTriangleList;

	// ViewportStateCreateInfo
	vk::Viewport mViewport;
	vk::Rect2D mScissor;

	vk::PipelineViewportStateCreateInfo buildPipelineViewportState() const;
};

