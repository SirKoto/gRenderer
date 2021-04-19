
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Mesh.h"
#include "../control/FrameContext.h"
#include "../graphics/RenderContext.h"

#include <fstream>
#include <imgui/imgui.h>
#include <tiny_obj_loader/tiny_obj_loader.h>
#include <tiny_ply_loader/tinyply.h>
#include <unordered_map>
#include <filesystem>

namespace gr
{

void Mesh::load(FrameContext* fc,
	const char* filePath)
{

	std::filesystem::path path(filePath);
	if (path.is_absolute()) {
		path = std::filesystem::relative(path, fc->gc().getProjectPath());
	}

	mPath.assign(path.string());
	
	std::filesystem::path absolutePath = fc->gc().getProjectPath() / path;

	vkg::RenderContext* rc = &fc->rc();

	mVertices.clear();
	mIndices.clear();
	mBBox.reset();

	if (mPath.find(".obj") != std::string::npos) {
		parseObj(absolutePath.string().c_str());
	}
	else if (mPath.find(".ply") != std::string::npos) {
		parsePly(absolutePath.string().c_str());
	}


	// create buffers
	{
		if (mIndexBuffer) {
			throw std::logic_error("Error! Buffer previously alocated");
		}
		mIndexBufferSize = sizeof(uint32_t) * mIndices.size();
		mIndexBuffer = rc->createIndexBuffer(mIndexBufferSize);
		if (mVertexBuffer) {
			throw std::logic_error("Error! Buffer previously alocated");
		}
		mVertexBufferSize = sizeof(Vertex) * mVertices.size();
		mVertexBuffer = rc->createVertexBuffer(mVertexBufferSize);
	}

	// upload to gpu
	rc->getTransferer()->transferToBuffer(*rc,
		mVertices.data(), mVertices.size() * sizeof(mVertices[0]),
		mVertexBuffer);
	rc->getTransferer()->transferToBuffer(*rc,
		mIndices.data(), mIndices.size() * sizeof(mIndices[0]),
		mIndexBuffer);
}

void Mesh::scheduleDestroy(FrameContext* fc)
{
	if (mIndexBuffer) {
		fc->scheduleToDestroy(mIndexBuffer);
		mIndexBuffer = nullptr;
	}
	if (mVertexBuffer) {
		fc->scheduleToDestroy(mVertexBuffer);
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
		.addAttributeFloat(1, 3, offsetof(Vertex, Vertex::normal))
		.addAttributeFloat(2, 3, offsetof(Vertex, Vertex::color))
		.addAttributeFloat(3, 2, offsetof(Vertex, Vertex::texCoord));
}

void Mesh::parseObj(const char* fileName)
{

	tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig readerConfig;
	readerConfig.triangulate = true;
	readerConfig.vertex_color = true;

	if (!reader.ParseFromFile(fileName, readerConfig)) {
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
			if (idx.texcoord_index >= 0) {
				v.texCoord = {
					vertAttribs.texcoords[2 * idx.texcoord_index + 0],
					vertAttribs.texcoords[2 * idx.texcoord_index + 1]
				};
			}
			if (idx.normal_index >= 0) {
				v.normal = {
					vertAttribs.normals[3 * idx.normal_index + 0],
					vertAttribs.normals[3 * idx.normal_index + 1],
					vertAttribs.normals[3 * idx.normal_index + 2]
				};
			}
			v.color = {
				vertAttribs.colors[3 * idx.vertex_index + 0],
				vertAttribs.colors[3 * idx.vertex_index + 1],
				vertAttribs.colors[3 * idx.vertex_index + 2]
			};

			const decltype(mVerticesCache)::const_iterator it =
				mVerticesCache.find(v);
			if (it != mVerticesCache.end()) {
				mIndices.push_back(it->second);
			}
			else {
				mIndices.push_back(static_cast<uint32_t>(mVertices.size()));
				mVertices.push_back(v);
				mVerticesCache.emplace(v, mIndices.back());
				mBBox.addPoint(v.pos);
			}
		}
	}
}

void Mesh::parsePly(const char* fileName)
{

	std::ifstream stream(fileName, std::ios::binary);

	if (!stream) {
		throw std::runtime_error("Error: Can't open file " + std::string(fileName));
	}

	tinyply::PlyFile file;
	bool res = file.parse_header(stream);
	if (!res) {
		throw std::runtime_error("Error: Can't parse ply header.");
	}

	std::shared_ptr<tinyply::PlyData> vertices, normals, texcoords, faces;
	try { vertices = file.request_properties_from_element("vertex", { "x", "y", "z" }); }
	catch (const std::exception&) {}

	try { normals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" }); }
	catch (const std::exception&) {}

	try { texcoords = file.request_properties_from_element("vertex", { "u", "v" }); }
	catch (const std::exception&) {}

	try { faces = file.request_properties_from_element("face", { "vertex_indices" }, 3); }
	catch (const std::exception&) {}

	file.read(stream);

	if (!vertices || !faces) {
		throw std::runtime_error("Error: Can't load faces of ply.");
	}

	assert(vertices->t == tinyply::Type::FLOAT32);
	assert(!normals || normals->t == tinyply::Type::FLOAT32);
	assert(!texcoords || texcoords->t == tinyply::Type::FLOAT32);


	// copy vertices
	mVertices.resize(vertices->count);
	for (size_t i = 0; i < vertices->count; ++i) {
		std::memcpy(&mVertices[i].pos, vertices->buffer.get() + i * 3 * sizeof(float), 3 * sizeof(float));
		if (texcoords) {
			std::memcpy(&mVertices[i].texCoord, texcoords->buffer.get() + i * 2 * sizeof(float), 2 * sizeof(float));
		}
		if (normals) {
			std::memcpy(&mVertices[i].normal, normals->buffer.get() + i * 3 * sizeof(float), 3 * sizeof(float));
		}

		mBBox.addPoint(mVertices[i].pos);
	}

	mIndices.resize(faces->count * 3);
	if (faces->t == tinyply::Type::UINT32 || faces->t == tinyply::Type::INT32) {
		std::memcpy(mIndices.data(), faces->buffer.get(), faces->buffer.size_bytes());
	}
	else if (faces->t == tinyply::Type::UINT16 || faces->t == tinyply::Type::INT16) {
		for (size_t i = 0; i < faces->count; ++i) {
			glm::i16vec3 tmp;
			std::memcpy(&tmp, faces->buffer.get() + i * 3 * sizeof(uint16_t), 3 * sizeof(uint16_t));
			mIndices[3 * i + 0] = static_cast<uint32_t>(tmp.x);
			mIndices[3 * i + 1] = static_cast<uint32_t>(tmp.y);
			mIndices[3 * i + 2] = static_cast<uint32_t>(tmp.z);
		}
	}
	else {
		throw std::runtime_error("Error: Cant read face format");
	}

}

bool Mesh::Vertex::operator==(const Vertex& o) const
{
	return this->pos == o.pos &&
		this->normal == o.normal &&
		this->color == o.color &&
		this->texCoord == o.texCoord;
}

std::size_t Mesh::VertexHash::operator()(const Vertex& o) const
{
	return ((std::hash<glm::vec3>()(o.pos) ^
		(std::hash<glm::vec3>()(o.normal)) ^
		(std::hash<glm::vec3>()(o.color) << 1)) >> 1) ^
		(std::hash<glm::vec2>()(o.texCoord) << 1);
}

void Mesh::renderImGui(FrameContext* fc, GuiFeedback* feedback)
{
	ImGui::TextDisabled("Triangle Mesh");
	ImGui::Separator();
	ImGui::Text("Num vertices: %u", mVertices.size());
	ImGui::Text("Num indices: %u", mIndices.size());

	ImGui::Text("BBox\n\t(%.2f,%.2f,%.2f)\n\t(%.2f,%.2f,%.2f)", 
		mBBox.getMin().x, mBBox.getMin().y, mBBox.getMin().z,
		mBBox.getMax().x, mBBox.getMax().y, mBBox.getMax().z);
	ImGui::Text("BBox size (%.2f, %.2f, %.2f)",
		mBBox.getSize().x, mBBox.getSize().y, mBBox.getSize().z);

	ImGui::Separator();
	ImGui::Text("Path of model:");
	ImGui::InputText(
		"##Path",
		const_cast<char*>(mPath.c_str()),
		mPath.size(),
		ImGuiInputTextFlags_ReadOnly
	);

}

void Mesh::start(FrameContext* fc)
{
	if (!mPath.empty()) {
		this->load(fc, mPath.c_str());
	}
}

} // namespace gr
