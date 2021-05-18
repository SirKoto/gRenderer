#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "../graphics/resources/Buffer.h"
#include "../graphics/shaders/VertexInputDescription.h"
#include "IObject.h"
#include "../utils/math/BBox.h"

namespace gr
{

// Forward declare render context
namespace vkg {
class RenderContext;
}
class FrameContext;
class GlobalContext;

class Mesh : public IObject
{
public:

	Mesh() = default;

	Mesh(const Mesh&) = default;
	Mesh(Mesh&&) = default;
	Mesh& operator=(const Mesh&) = default;
	Mesh& operator=(Mesh&&) = default;

	// Catch exception if it can fail
	void load(FrameContext* fc,
		const char* filePath);

	void scheduleDestroy(FrameContext* fc) override final;
	void renderImGui(FrameContext* fc, Gui* gui) override final;
	void start(FrameContext* fc) override final;

	static constexpr const char* s_getClassName() { return "Mesh"; }


	const vk::Buffer& getVB() const { return mVertexBuffer.getVkBuffer(); }
	const vk::Buffer& getIB() const { return mIndexBuffer.getVkBuffer(); }

	uint32_t getNumIndices() const { return static_cast<uint32_t>(mIndices.size()); }
	uint32_t getNumLODs() const { return (uint32_t) mLODsDrawData.size(); }
	uint32_t getNumIndicesLod(uint32_t lod) const { return static_cast<uint32_t>(mLODs.at(lod).indices.size()); }
	uint32_t getDepthLod(uint32_t lod) const { return mLODs.at(lod).depth; }

	void getDrawDataLod(uint32_t lod, uint32_t* numIndices, uint32_t* firstIndex, vk::DeviceSize* vertexOffset) const;

	const mth::AABBox& getBBox() const { return mBBox; }

	// add binding with locations:
	// (location = 0) float3 vertexPosition
	// (location = 1) float3 normal
	// (location = 2) float3 vertexColor
	// (location = 3) float2 texCoord
	static void addToVertexInputDescription(uint32_t binding,
		vkg::VertexInputDescription* vid);

	operator bool()const { return mIndexBuffer; }

	
	void regenerateLODs(FrameContext* fc, bool useQuadricErrorMetric = false, bool useNormalClustering = false);

protected:

	struct Vertex {
		glm::vec3 pos = glm::vec3(0.0f);
		glm::vec3 color = glm::vec3(0.0f);
		glm::vec3 normal = glm::vec3(0.0f);
		glm::vec2 texCoord = glm::vec2(0.0f);

		bool operator==(const Vertex&) const;
	};

	struct VertexHash {
		std::size_t operator()(const Vertex& o) const;
	};


	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices;

	struct LOD {
		uint32_t depth;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};
	std::vector<LOD> mLODs;
	struct LOD_DrawData {
		uint32_t numIndices;
		uint32_t firstIndex;
		vk::DeviceSize vertexOffset;
	};
	std::vector<LOD_DrawData> mLODsDrawData;
	

	vkg::Buffer mIndexBuffer;
	vkg::Buffer mVertexBuffer;

	vk::DeviceSize mVertexBufferSize = 0;
	vk::DeviceSize mIndexBufferSize = 0;

	mth::AABBox mBBox;

	std::string mPath;


	void parseObj(const char* fileName);
	void parsePly(const char* fileName);

	void uploadDataToGPU(FrameContext* fc);

	// Serialization functions
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(cereal::base_class<IObject>(this));
		archive(GR_SERIALIZE_NVP_MEMBER(mPath));
	}

	GR_SERIALIZE_PRIVATE_MEMBERS
};

} // namespace gr

GR_SERIALIZE_TYPE(gr::Mesh)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::Mesh)