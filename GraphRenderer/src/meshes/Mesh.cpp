
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Mesh.h"
#include "../control/FrameContext.h"
#include "../graphics/RenderContext.h"

#include <tiny_obj_loader/tiny_obj_loader.h>
#include<unordered_map>

namespace gr
{



void Mesh::load(vkg::RenderContext* rc,
	const char* filePath)
{

	mPath.assign(filePath);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig readerConfig;
	readerConfig.triangulate = true;
	readerConfig.vertex_color = true;

	if (!reader.ParseFromFile(mPath, readerConfig)) {
		throw std::runtime_error("Mesh Load Error: " +
			reader.Error() + "\n" + reader.Warning());
	}

	std::unordered_map<Vertex, uint32_t, VertexHash> mVerticesCache;

	const tinyobj::attrib_t& vertAttribs = reader.GetAttrib();
	const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
	const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

	for (const tinyobj::shape_t& shape : shapes) {
		const tinyobj::mesh_t& mesh = shape.mesh;

		for (const tinyobj::index_t& idx : mesh.indices) {
			Vertex v;
			v.pos = {
				vertAttribs.vertices[3 * idx.vertex_index + 0],
				vertAttribs.vertices[3 * idx.vertex_index + 1],
				vertAttribs.vertices[3 * idx.vertex_index + 2]
			};
			v.texCoord = {
				vertAttribs.texcoords[2 * idx.texcoord_index + 0],
				vertAttribs.texcoords[2 * idx.texcoord_index + 1]
			};
			v.color = {
				vertAttribs.colors[3 * idx.vertex_index + 0],
				vertAttribs.colors[3 * idx.vertex_index + 1],
				vertAttribs.colors[3 * idx.vertex_index + 2]
			};

			const decltype(mVerticesCache)::const_iterator it =
				mVerticesCache.find(v);
			if (it != mVerticesCache.end()) {
				indices.push_back(it->second);
			}
			else {
				indices.push_back(static_cast<uint32_t>(vertices.size()));
				vertices.push_back(v);
				mVerticesCache.emplace(v, indices.back());
			}
		}
	}

	mNumIndices = static_cast<uint32_t>(indices.size());
	mNumVertices = static_cast<uint32_t>(vertices.size());

	// create buffers
	{
		if (mIndexBuffer) {
			throw std::logic_error("Error! Buffer previously alocated");
		}
		mIndexBufferSize = sizeof(uint32_t) * mNumIndices;
		mIndexBuffer = rc->createIndexBuffer(mIndexBufferSize);
		if (mVertexBuffer) {
			throw std::logic_error("Error! Buffer previously alocated");
		}
		mVertexBufferSize = sizeof(Vertex) * mNumVertices;
		mVertexBuffer = rc->createVertexBuffer(mVertexBufferSize);
	}

	// upload to gpu
	rc->getTransferer()->transferToBuffer(*rc,
		vertices.data(), vertices.size() * sizeof(vertices[0]),
		mVertexBuffer);
	rc->getTransferer()->transferToBuffer(*rc,
		indices.data(), indices.size() * sizeof(indices[0]),
		mIndexBuffer);
}

void Mesh::scheduleDestroy(FrameContext* fc)
{
	if (mIndexBuffer) {
		fc->scheduleToDelete(mIndexBuffer);
		mIndexBuffer = nullptr;
	}
	if (mVertexBuffer) {
		fc->scheduleToDelete(mVertexBuffer);
		mVertexBuffer = nullptr;
	}
}

void Mesh::destroy(vkg::RenderContext* rc)
{
	if (mIndexBuffer) {
		rc->destroy(mIndexBuffer);
		mIndexBuffer = nullptr;
	}
	if (mVertexBuffer) {
		rc->destroy(mVertexBuffer);
		mVertexBuffer = nullptr;
	}
}


void Mesh::addToVertexInputDescription(
	uint32_t binding,
	vkg::VertexInputDescription* vid)
{
	if (vid->existsBinding(binding)) {
		throw std::logic_error("Error, already exists binding with such id!");
	}

	vid->addBinding(binding, sizeof(Vertex))
		.addAttributeFloat(0, 3, offsetof(Vertex, Vertex::pos))
		.addAttributeFloat(1, 3, offsetof(Vertex, Vertex::color))
		.addAttributeFloat(2, 2, offsetof(Vertex, Vertex::texCoord));
}

bool Mesh::Vertex::operator==(const Vertex& o) const
{
	return this->pos == o.pos &&
		this->color == o.color &&
		this->texCoord == o.texCoord;
}

std::size_t Mesh::VertexHash::operator()(const Vertex& o) const
{
	return ((std::hash<glm::vec3>()(o.pos) ^
		(std::hash<glm::vec3>()(o.color) << 1)) >> 1) ^
		(std::hash<glm::vec2>()(o.texCoord) << 1);
}

} // namespace gr
