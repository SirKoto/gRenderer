
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Mesh.h"
#include "../control/FrameContext.h"
#include "../graphics/RenderContext.h"
#include "../gui/GuiUtils.h"

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
	
	std::filesystem::path absolutePath = fc->gc().getAbsolutePathTo(path);

	vkg::RenderContext* rc = &fc->rc();

	mVertices.clear();
	mIndices.clear();
	mBBox.reset();

	if (mPath.find(".obj") != std::string::npos) {
		parseObj(absolutePath.string().c_str(), &mVertices, &mIndices, &mBBox);
	}
	else if (mPath.find(".ply") != std::string::npos) {
		parsePly(absolutePath.string().c_str(), &mVertices, &mIndices, &mBBox);
	}

	uint32_t lod_i = 0;
	for (LOD& lod : mLODs) {
		std::filesystem::path fileLod = fc->gc().getAbsolutePathTo(getRelativeLodPath(lod_i++));
		if (std::filesystem::exists(fileLod)) {
			parsePly(fileLod.string().c_str(), &lod.vertices, &lod.indices);
		}
		else {
			lod.vertices.clear();
			lod.indices.clear();
		}
	}

	this->uploadDataToGPU(fc);
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

	mLODsDrawData.clear();
}


void Mesh::getDrawDataLod(uint32_t lod, uint32_t* numIndices, uint32_t* firstIndex, vk::DeviceSize* vertexOffset) const
{
	assert(numIndices != nullptr && firstIndex != nullptr && vertexOffset != nullptr);

	if (lod == 0) {
		*numIndices = this->getNumIndices();
		*firstIndex = 0;
		*vertexOffset = 0;
	}
	else {
		*numIndices = mLODsDrawData[lod - 1].numIndices;
		*firstIndex = mLODsDrawData[lod - 1].firstIndex;
		*vertexOffset = mLODsDrawData[lod - 1].vertexOffset;
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
		//.addAttributeFloat(2, 3, offsetof(Vertex, Vertex::color))
		//.addAttributeFloat(3, 2, offsetof(Vertex, Vertex::texCoord))
		;
}

void Mesh::parseObj(const char* fileName, std::vector<Vertex>* outVertices, std::vector<uint32_t>* outIndices, mth::AABBox* outBBox)
{
	assert(outVertices != nullptr && outIndices != nullptr);

	tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig readerConfig;
	readerConfig.triangulate = true;
	readerConfig.vertex_color = true;

	if (!reader.ParseFromFile(fileName, readerConfig)) {
		throw std::runtime_error("Mesh Load Error: " +
			reader.Error() + "\n" + reader.Warning());
	}

	outIndices->clear();
	outVertices->clear();
	if (outBBox != nullptr) {
		outBBox->reset();
	}

	std::unordered_map<Vertex, uint32_t, VertexHash> verticesCache;

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
			//if (idx.texcoord_index >= 0) {
			//	v.texCoord = {
			//		vertAttribs.texcoords[2 * idx.texcoord_index + 0],
			//		vertAttribs.texcoords[2 * idx.texcoord_index + 1]
			//	};
			//}
			if (idx.normal_index >= 0) {
				v.normal = {
					vertAttribs.normals[3 * idx.normal_index + 0],
					vertAttribs.normals[3 * idx.normal_index + 1],
					vertAttribs.normals[3 * idx.normal_index + 2]
				};
			}
			//v.color = {
			//	vertAttribs.colors[3 * idx.vertex_index + 0],
			//	vertAttribs.colors[3 * idx.vertex_index + 1],
			//	vertAttribs.colors[3 * idx.vertex_index + 2]
			//};

			const decltype(verticesCache)::const_iterator it =
				verticesCache.find(v);
			if (it != verticesCache.end()) {
				outIndices->push_back(it->second);
			}
			else {
				outIndices->push_back(static_cast<uint32_t>(outVertices->size()));
				outVertices->push_back(v);
				verticesCache.emplace(v, outIndices->back());
				if (outBBox != nullptr) {
					outBBox->addPoint(v.pos);
				}
			}
		}
	}
}

void Mesh::parsePly(const char* fileName, std::vector<Vertex>* outVertices, std::vector<uint32_t>* outIndices, mth::AABBox* outBBox)
{
	assert(outVertices != nullptr && outIndices != nullptr);
	std::ifstream stream(fileName, std::ios::binary);

	if (!stream) {
		throw std::runtime_error("Error: Can't open file " + std::string(fileName));
	}

	tinyply::PlyFile file;
	bool res = file.parse_header(stream);
	if (!res) {
		throw std::runtime_error("Error: Can't parse ply header.");
	}

	bool recomputeNormals = false;

	std::shared_ptr<tinyply::PlyData> vertices, normals, texcoords, faces;
	try { vertices = file.request_properties_from_element("vertex", { "x", "y", "z" }); }
	catch (const std::exception&) {}

	try { normals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" }); }
	catch (const std::exception&) { recomputeNormals = true; }

	try { texcoords = file.request_properties_from_element("vertex", { "u", "v" }); }
	catch (const std::exception&) {}
	try { texcoords = file.request_properties_from_element("vertex", { "s", "t" }); }
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
	if (outBBox != nullptr) {
		outBBox->reset();
	}
	// copy vertices
	outVertices->resize(vertices->count);
	for (size_t i = 0; i < vertices->count; ++i) {
		std::memcpy(&(*outVertices)[i].pos, vertices->buffer.get() + i * 3 * sizeof(float), 3 * sizeof(float));
		//if (texcoords) {
		//	std::memcpy(&(*outVertices)[i].texCoord, texcoords->buffer.get() + i * 2 * sizeof(float), 2 * sizeof(float));
		//}
		if (normals) {
			std::memcpy(&(*outVertices)[i].normal, normals->buffer.get() + i * 3 * sizeof(float), 3 * sizeof(float));
		}
		if (outBBox != nullptr) {
			outBBox->addPoint((*outVertices)[i].pos);
		}
	}

	outIndices->resize(faces->count * 3);
	if (faces->t == tinyply::Type::UINT32 || faces->t == tinyply::Type::INT32) {
		std::memcpy(outIndices->data(), faces->buffer.get(), faces->buffer.size_bytes());
	}
	else if (faces->t == tinyply::Type::UINT16 || faces->t == tinyply::Type::INT16) {
		for (size_t i = 0; i < faces->count; ++i) {
			glm::i16vec3 tmp;
			std::memcpy(&tmp, faces->buffer.get() + i * 3 * sizeof(uint16_t), 3 * sizeof(uint16_t));
			(*outIndices)[3 * i + 0] = static_cast<uint32_t>(tmp.x);
			(*outIndices)[3 * i + 1] = static_cast<uint32_t>(tmp.y);
			(*outIndices)[3 * i + 2] = static_cast<uint32_t>(tmp.z);
		}
	}
	else {
		throw std::runtime_error("Error: Cant read face format");
	}

	if (!normals || recomputeNormals) {
		computeNormals(*outIndices, outVertices);
	}

}

void Mesh::computeNormals(const std::vector<uint32_t>& indices, std::vector<Vertex>* outVertices)
{
	// Compute the planes of all triangles
	std::vector<glm::vec3> triangleNormals(indices.size() / 3);
	for (uint32_t t = 0; t < (uint32_t)indices.size() / 3; ++t) {
		const glm::vec3& v0 = (*outVertices)[indices[3 * t + 0]].pos;
		const glm::vec3& v1 = (*outVertices)[indices[3 * t + 1]].pos;
		const glm::vec3& v2 = (*outVertices)[indices[3 * t + 2]].pos;

		glm::vec3 n = glm::cross(v1 - v0, v2 - v0);
		triangleNormals[t] = glm::normalize(n);
	}

	// Compute V:{F}
	std::vector<std::vector<uint32_t>> vert2faces(outVertices->size());
	std::vector<uint32_t> vertexArity(outVertices->size(), 0);
	for (uint32_t t = 0; t < (uint32_t)indices.size() / 3; ++t) {
		vertexArity[indices[3 * t + 0]] += 1;
		vertexArity[indices[3 * t + 1]] += 1;
		vertexArity[indices[3 * t + 2]] += 1;
	}
	for (uint32_t v = 0; v < (uint32_t)outVertices->size(); ++v) {
		vert2faces[v].reserve(vertexArity[v]);
	}
	for (uint32_t t = 0; t < (uint32_t)indices.size() / 3; ++t) {
		vert2faces[indices[3 * t + 0]].push_back(t);
		vert2faces[indices[3 * t + 1]].push_back(t);
		vert2faces[indices[3 * t + 2]].push_back(t);
	}

	for (uint32_t v = 0; v < (uint32_t)outVertices->size(); ++v) {
		(*outVertices)[v].normal = glm::vec3(0);

		for (const uint32_t& f : vert2faces[v]) {
			(*outVertices)[v].normal += triangleNormals[f];
		}
		(*outVertices)[v].normal /= vert2faces[v].size();
	}
}

void Mesh::uploadDataToGPU(FrameContext* fc)
{

	// destroy buffers if exist there
	scheduleDestroy(fc);

	vkg::RenderContext* rc = &fc->rc();
	// create buffers
	{
		if (mIndexBuffer) {
			throw std::logic_error("Error! Buffer previously alocated");
		}
		mIndexBufferSize = sizeof(uint32_t) * mIndices.size();
		for (const LOD& lod : mLODs) {
			mIndexBufferSize += sizeof(uint32_t) * lod.indices.size();
		}
		mIndexBuffer = rc->createIndexBuffer(mIndexBufferSize);
		if (mVertexBuffer) {
			throw std::logic_error("Error! Buffer previously alocated");
		}
		mVertexBufferSize = sizeof(Vertex) * mVertices.size();
		for (const LOD& lod : mLODs) {
			mVertexBufferSize += sizeof(Vertex) * lod.vertices.size();
		}
		mVertexBuffer = rc->createVertexBuffer(mVertexBufferSize);
	}

	// upload to gpu
	rc->getTransferer()->transferToBuffer(*rc,
		mVertices.data(), mVertices.size() * sizeof(mVertices[0]),
		mVertexBuffer);
	rc->getTransferer()->transferToBuffer(*rc,
		mIndices.data(), mIndices.size() * sizeof(mIndices[0]),
		mIndexBuffer);

	vk::DeviceSize vertexOffset = mVertices.size() * sizeof(mVertices[0]);
	vk::DeviceSize indexOffset = mIndices.size() * sizeof(mIndices[0]);
	uint32_t nextFirstIndice = static_cast<uint32_t>(mIndices.size());
	mLODsDrawData.resize(mLODs.size());
	for (uint32_t i = 0; i < (uint32_t)mLODs.size(); ++i) {

		if (mLODs[i].indices.empty()) {
			continue;
		}

		rc->getTransferer()->transferToBuffer(*rc,
			mLODs[i].vertices.data(), mLODs[i].vertices.size() * sizeof(mLODs[i].vertices[0]),
			mVertexBuffer, vertexOffset);
		rc->getTransferer()->transferToBuffer(*rc,
			mLODs[i].indices.data(), mLODs[i].indices.size() * sizeof(mLODs[i].indices[0]),
			mIndexBuffer, indexOffset);

		mLODsDrawData[i].firstIndex = nextFirstIndice;
		mLODsDrawData[i].numIndices = static_cast<uint32_t>(mLODs[i].indices.size());
		mLODsDrawData[i].vertexOffset = vertexOffset;

		vertexOffset += mLODs[i].vertices.size() * sizeof(mLODs[i].vertices[0]);
		indexOffset += mLODs[i].indices.size() * sizeof(mLODs[i].indices[0]);
		nextFirstIndice += static_cast<uint32_t>(mLODs[i].indices.size());
	}
}

void Mesh::saveLODModels(FrameContext* fc) const
{
	{
		std::string log = "Saving " + std::to_string(mLODs.size()) + std::string(" LODs of ") + this->getObjectName();
		fc->gc().addNewLog(log);
	}
	const auto start_timer = std::chrono::high_resolution_clock::now();

	std::ofstream stream;
	
	for (uint32_t i = 0; i < (uint32_t)mLODs.size(); ++i) {
		const auto start_inner_timer = std::chrono::high_resolution_clock::now();

		std::filesystem::path modelPath = fc->gc().getAbsolutePathTo( getRelativeLodPath(i) );
		stream.open(modelPath, std::ofstream::trunc | std::ofstream::binary);
		if (!stream) {
			throw std::runtime_error("Error: Can't store LOD model");
		}
		const LOD& lod = mLODs[i];

		tinyply::PlyFile file;
		// vertex positions
		std::vector<glm::vec3> vert(lod.vertices.size());
		{
			for (uint32_t j = 0; j < (uint32_t)lod.vertices.size(); ++j) {
				vert[j] = lod.vertices[j].pos;
			}
			file.add_properties_to_element(
				"vertex", { "x", "y", "z" },
				tinyply::Type::FLOAT32,
				vert.size(),
				reinterpret_cast<uint8_t*>(vert.data()),
				tinyply::Type::INVALID, 0);
		}
		// vertex normals
		std::vector<glm::vec3> norm(lod.vertices.size());
		{
			for (uint32_t j = 0; j < (uint32_t)lod.vertices.size(); ++j) {
				norm[j] = lod.vertices[j].normal;
			}
			file.add_properties_to_element(
				"vertex", { "nx", "ny", "nz" },
				tinyply::Type::FLOAT32,
				norm.size(),
				reinterpret_cast<const uint8_t*>(norm.data()),
				tinyply::Type::INVALID, 0);
		}
		// faces
		{
			file.add_properties_to_element(
				"face", { "vertex_indices" },
				tinyply::Type::UINT32, lod.indices.size() / 3,
				reinterpret_cast<const uint8_t*>(lod.indices.data()),
				tinyply::Type::UINT8, 3);
		}

		file.write(stream, true);

		stream.close();

		const auto end_inner_timer = std::chrono::high_resolution_clock::now();
		typedef std::chrono::duration<double_t> Fsec;

		Fsec dur = end_inner_timer - start_inner_timer;
		std::stringstream ss;
		ss << "Stored Lod "<< i << ", computed at depth " << lod.depth << ", in " << modelPath << '\n';
		ss << "\tTook " << dur.count() << " seconds";
		fc->gc().addNewLog(ss.str());
	}

	const auto end_timer = std::chrono::high_resolution_clock::now();
	typedef std::chrono::duration<double_t> Fsec;

	Fsec dur = end_timer - start_timer;
	std::stringstream ss;
	ss << "All lods stored" << '\n';
	ss << "\tTook " << dur.count() << " seconds\n";
	fc->gc().addNewLog(ss.str());

}

std::string Mesh::getRelativeLodPath(uint32_t lod) const
{
	assert(lod < (uint32_t)mLODs.size());

	std::filesystem::path path = std::filesystem::path(mPath).parent_path();
	std::filesystem::path fileName = std::filesystem::path(mPath).filename();

	std::filesystem::path newFile = path / (fileName.stem().string() + ".lod" + std::to_string(mLODs[lod].depth) + ".ply");
	return newFile.string();
}

bool Mesh::Vertex::operator==(const Vertex& o) const
{
	return this->pos == o.pos &&
		this->normal == o.normal;//&&
		//this->color == o.color &&
		//this->texCoord == o.texCoord;
}

std::size_t Mesh::VertexHash::operator()(const Vertex& o) const
{
	return ((std::hash<glm::vec3>()(o.pos) ^
		(std::hash<glm::vec3>()(o.normal) << 1)));//^
		//(std::hash<glm::vec3>()(o.color) << 1)) >> 1) ^
		//(std::hash<glm::vec2>()(o.texCoord) << 1);
}

void Mesh::renderImGui(FrameContext* fc, Gui* gui)
{
	ImGui::TextDisabled("Triangle Mesh");
	ImGui::Separator();
	ImGui::Text("Num vertices: %u", mVertices.size());
	ImGui::Text("Num indices: %u", mIndices.size());

	
	ImGui::Separator();
	if (ImGui::TreeNode("Levels of detail:")) {
		bool minus = ImGui::Button("-"); ImGui::SameLine();
		ImGui::TextDisabled("%u", static_cast<uint32_t>(mLODs.size())); ImGui::SameLine();
		bool plus = ImGui::Button("+");
		if (minus && !mLODs.empty()) {
			mLODs.resize(mLODs.size() - 1);
		}
		if (plus) {
			mLODs.resize(mLODs.size() + 1);
		}

		ImGui::PushID("Lods");
		for (size_t i = 0; i < mLODs.size(); ++i) {
			if (ImGui::TreeNode((void*)(intptr_t)i,"LOD %d", i + 1)) {
				ImGui::Text("Num vertices: %u", mLODs[i].vertices.size());
				ImGui::Text("Num indices: %u", mLODs[i].indices.size());
				ImGui::Text("Computed on depth: %u", mLODs[i].depth);
				std::string str = std::string("Depth##") + std::to_string(i);
				int32_t step = 1;
				ImGui::InputScalar(str.c_str(), ImGuiDataType_U32, (void*)&mLODs[i].depth, &step, nullptr, "%d", ImGuiInputTextFlags_None);

				gui::helpMarker("The depth marks how much detail should be preserved. Consecutive LODs must have decreasing depth.");

				if (i > 0) {
					mLODs[i].depth = std::min(mLODs[i].depth, std::min(mLODs[i - 1].depth, mLODs[i - 1].depth - 1));
				}
				ImGui::TreePop();
			}
		}

		if (ImGui::TreeNode("Regenerate LOD")) {
			if (ImGui::Button("Representative Mean")) {
				this->regenerateLODs(fc);
				this->uploadDataToGPU(fc);
			}
			if (ImGui::Button("Representative Mean, Normal Clustering")) {
				this->regenerateLODs(fc, false, true);
				this->uploadDataToGPU(fc);
			}
			if (ImGui::Button("Quadric Error Metric")) {
				this->regenerateLODs(fc, true, false);
				this->uploadDataToGPU(fc);
			}
			if (ImGui::Button("Quadric Error Metric, Normal Clustering")) {
				this->regenerateLODs(fc, true, true);
				this->uploadDataToGPU(fc);
			}

			ImGui::TreePop();
		}

		if (ImGui::Button("Save LODs")) {
			this->saveLODModels(fc);
		}
		gui::helpMarker("Save the newly generated LODs into models in the same folder as the model");


		ImGui::TreePop();
		ImGui::PopID();
	}

	ImGui::Separator();
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
