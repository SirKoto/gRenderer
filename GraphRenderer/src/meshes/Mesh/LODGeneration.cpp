#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "../Mesh.h"


#include <bitset>

struct Node
{
	std::bitset<8> nodeTypes{ 0 };
};

struct Task {
	std::vector<uint32_t> vertices;
	uint32_t depth;
	glm::vec3 midCoord;
};

void gr::Mesh::regenerateLODs(FrameContext* fc)
{
	//std::vector<Node> nodes;

	std::stack<Task> tasks;

	if (mLODs.empty()) {
		return;
	}

	// map depth to index of LOD
	std::map<uint32_t, uint32_t> depthToLod;
	for (uint32_t i = 0; i < (uint32_t)mLODs.size(); ++i) {
		mLODs[i].indices.clear();
		mLODs[i].vertices.clear();
		depthToLod.insert({ mLODs[i].depth, i });
	}

	uint32_t maxDepth = mLODs.front().depth;

	const float octreeSize = std::max(mBBox.getSize().x, std::max(mBBox.getSize().y, mBBox.getSize().z));


	{
		Task root;
		root.vertices.resize(mVertices.size());
		std::iota(root.vertices.begin(), root.vertices.end(), 0);
		root.depth = 0;
		root.midCoord = (mBBox.getMin() + octreeSize * 0.5f);
		tasks.push(std::move(root));
	}


	std::array<std::vector<uint32_t>, 8> childsVert = {};
	for (auto& v : childsVert) v.reserve(mVertices.size() / 6);

	std::vector<std::vector<uint32_t>> old2newVerticesInLod;
	old2newVerticesInLod.resize(mLODs.size());
	for (auto& v : old2newVerticesInLod) v.resize(mVertices.size());


	while (!tasks.empty()) {

		const Task task = std::move(tasks.top());
		tasks.pop();

		float size = octreeSize / static_cast<float>(1 << task.depth);

		for (auto& v : childsVert) v.clear();

		for (const uint32_t i : task.vertices) {
			const glm::vec3 dir = mVertices[i].pos - task.midCoord;
			uint32_t k = 
						((dir.x >= 0.f ? 1 : 0) << 0) +
						((dir.y >= 0.f ? 1 : 0) << 1) +
						((dir.z >= 0.f ? 1 : 0) << 2);
			childsVert[k].push_back(i);
		}

		// keep generating nodes....
		if (maxDepth  > task.depth) {
			Node node;

			for (uint32_t k = 0; k < 8; ++k) {
				node.nodeTypes[k] = !childsVert[k].empty();

				if (!childsVert[k].empty()) {
					glm::vec3 dir = { k & 0b1 ? 1 : -1, k & 0b10 ? 1 : -1, k & 0b100 ? 1 : -1 };
					Task newT;
					newT.depth = task.depth + 1;
					newT.midCoord = task.midCoord + 0.25f * size * dir;
					newT.vertices = childsVert[k];
					tasks.push(std::move(newT));
				}
			}

			// nodes.push_back(node);
		}

		// if this depth is one of the LODs, store vertex
		decltype(depthToLod)::const_iterator it = depthToLod.find(task.depth);
		if (it != depthToLod.end()) {
			Vertex v;
			v.pos = task.midCoord;
			v.normal = mVertices[task.vertices.front()].normal;
			uint32_t vertexIdx = (uint32_t) mLODs[it->second].vertices.size();
			mLODs[it->second].vertices.push_back(v);

			for (uint32_t i : task.vertices) {
				old2newVerticesInLod[it->second][i] = vertexIdx;
			}
		}

	}

	// create faces from indices
	std::unordered_set<glm::uvec3> alreadyAddedFaces;
	alreadyAddedFaces.reserve(mIndices.size() / 3);
	for (uint32_t lod = 0; lod < (uint32_t)mLODs.size(); ++lod) {
		alreadyAddedFaces.clear();

		for (uint32_t i = 0; i < mIndices.size() / 3; ++i) {
			glm::uvec3 f = {	old2newVerticesInLod[lod][mIndices[3 * i + 0]],
								old2newVerticesInLod[lod][mIndices[3 * i + 1]],
								old2newVerticesInLod[lod][mIndices[3 * i + 2]] };

			// if face has area, and does not exist yet
			if (f.x != f.y && f.x != f.z && f.y != f.z && alreadyAddedFaces.count(f) == 0) {
				alreadyAddedFaces.insert(f);
				mLODs[lod].indices.push_back(f.x);
				mLODs[lod].indices.push_back(f.y);
				mLODs[lod].indices.push_back(f.z);

			}

		}


	}

}