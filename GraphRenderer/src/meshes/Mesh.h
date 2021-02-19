#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "../graphics/resources/Buffer.h"
#include "../graphics/shaders/VertexInputDescription.h"

namespace gr
{

// Forward declare render context
namespace vkg {
class RenderContext;
}
class FrameContext;
class Mesh
{
public:

	Mesh() = default;
	Mesh(const Mesh&) = default;
	Mesh(Mesh&&) = default;
	Mesh& operator=(const Mesh&) = default;
	Mesh& operator=(Mesh&&) = default;

	void load(vkg::RenderContext* rc,
		const char* filePath);

	void scheduleDestroy(FrameContext* fc);
	void destroy(vkg::RenderContext* rc);


	const vk::Buffer& getVB() const { return mVertexBuffer.getVkBuffer(); }
	const vk::Buffer& getIB() const { return mIndexBuffer.getVkBuffer(); }

	uint32_t getNumIndices() const { return mNumIndices; }

	// add binding with locations:
	// (location = 0) float3 vertexPosition
	// (location = 1) float3 vertexColor
	// (location = 2) float2 texCoord
	static void addToVertexInputDescription(uint32_t binding,
		vkg::VertexInputDescription* vid);

	operator bool()const { return mIndexBuffer; }

protected:

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		bool operator==(const Vertex&) const;
	};

	struct VertexHash {
		std::size_t operator()(const Vertex& o) const;
	};

	vkg::Buffer mIndexBuffer;
	vkg::Buffer mVertexBuffer;

	uint32_t mNumIndices = 0;
	uint32_t mNumVertices = 0;
	vk::DeviceSize mVertexBufferSize = 0;
	vk::DeviceSize mIndexBufferSize = 0;

	std::string mPath;

};

} // namespace gr
