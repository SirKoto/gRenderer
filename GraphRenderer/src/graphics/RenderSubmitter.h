#pragma once

#include <vulkan/vulkan.hpp>
#include <unordered_map>

#include "resources/Buffer.h"

namespace gr
{

class FrameContext;
class GlobalContext;


namespace vkg
{


class RenderSubmitter
{
public:


	class DrawData {
	public:
		vk::Buffer vertexBuffer = nullptr;
		vk::DeviceSize vertexBufferOffset = 0;
		vk::Buffer indexBuffer = nullptr;
		uint32_t numIndices = 0;
		uint32_t firstIndex = 0;
		vk::DescriptorSet objectDescriptorSet = nullptr;
	};

	void pushPredefinedDraw(const DrawData& drawData);

	void setDefaultMaterial(
		const vk::Pipeline pipeline,
		const vk::PipelineLayout pipLayout,
		const vk::DescriptorSet descriptorSet
	);

	void setSceneDescriptorSet(
		const vk::DescriptorSet descriptor
	);

	void flushDraws(vk::CommandBuffer cmd);

private:

	typedef std::vector<DrawData> RenderList;

	struct MaterialKey {
		vk::Pipeline pipeline;
		vk::DescriptorSet materialDescriptorSet;

		bool operator==(const MaterialKey& o) const;

	};
	struct HashMaterial { 
		std::size_t operator()(const MaterialKey& b) const {
			return std::hash<vk::Pipeline>{}(b.pipeline) ^
				std::hash<vk::DescriptorSet>{}(b.materialDescriptorSet);
		}
	};
	struct Material {
		vk::PipelineLayout pipelineLayout;
		RenderList renderList;
	};

	typedef std::unordered_map<MaterialKey, Material, HashMaterial> MaterialRenderList;


	MaterialRenderList mMaterialRenderList;

	MaterialKey mDefaultMaterial;

	vk::DescriptorSet mSceneDescriptorSet;
};

}
}

