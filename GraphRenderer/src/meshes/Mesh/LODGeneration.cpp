#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "../Mesh.h"
#include "../../control/FrameContext.h"

#include <Eigen/Dense>
#include <Eigen/src/SVD/JacobiSVD.h>
#include <bitset>
#include <array>
#include <chrono>
#include <sstream>


typedef Eigen::Matrix4d Mat4;
typedef Eigen::Vector4d Vec4;

struct SixDirsTris {
	std::array<std::vector<uint32_t>, 6> vertices;

	bool empty() const {
		for (const std::vector<uint32_t>& v : vertices) if (!v.empty()) return false;
		return true;
	}

	static uint32_t classifyNormalDir(const glm::vec3& n) {

		if (std::abs(n.x) >= std::abs(n.y) && std::abs(n.x) >= std::abs(n.z)) {
			return n.x >= 0 ? 0 : 1;
		}
		if (std::abs(n.y) >= std::abs(n.x) && std::abs(n.y) >= std::abs(n.z)) {
			return n.y >= 0 ? 2 : 3;
		}

		return n.z >= 0 ? 4 : 5;
	}

	void push_back_normal(uint32_t idx, const glm::vec3& normal) {
		uint32_t dir = classifyNormalDir(normal);
		vertices[dir].push_back(idx);
	}

	void clear() {
		for (std::vector<uint32_t>& v : vertices) v.clear();
	}

	std::vector<uint32_t>& operator[](const uint32_t dir) {
		assert(dir < 6);
		return vertices[dir];
	}
	const std::vector<uint32_t>& operator[](const uint32_t dir) const {
		assert(dir < 6);
		return vertices[dir];
	}

	SixDirsTris& operator=(const SixDirsTris& o) {
		for (uint32_t dir = 0; dir < 6; ++dir) this->vertices[dir] = o.vertices[dir];
		return *this;
	}

};

struct OctNodeTask {
	SixDirsTris sixDirsTris;
	uint32_t depth;
	glm::vec3 midCoord;
};




void gr::Mesh::regenerateLODs(FrameContext* fc, bool useQuadricErrorMetric, bool useNormalClustering)
{

	std::stack<OctNodeTask> tasks;

	if (mLODs.empty()) {
		return;
	}

	const auto start_timer = std::chrono::high_resolution_clock::now();


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
		OctNodeTask root;
		// Add vertices in 6 directions
		if (useNormalClustering) {
			for (uint32_t i = 0; i < (uint32_t)mVertices.size(); ++i) {
				root.sixDirsTris.push_back_normal(i, mVertices[i].normal);
			}
		}
		else { // only add for the first normal cluster
			for (uint32_t i = 0; i < (uint32_t)mVertices.size(); ++i) {
				root.sixDirsTris.vertices.front().push_back(i);
			}
		}
		root.depth = 0;
		root.midCoord = (mBBox.getMin() + octreeSize * 0.5f);
		tasks.push(std::move(root));
	}


	// Only will use the 6 children if requested
	std::array<SixDirsTris, 8> childsVert = {};

	std::vector<std::vector<uint32_t>> old2newVerticesInLod;
	old2newVerticesInLod.resize(mLODs.size());
	for (auto& v : old2newVerticesInLod) v.resize(mVertices.size());


	// Compute the planes of all triangles
	std::vector<Vec4> trianglePlanes(mIndices.size() / 3);
	for (uint32_t t = 0; t < (uint32_t)mIndices.size() / 3; ++t) {
		const glm::vec3& v0 = mVertices[mIndices[3 * t + 0]].pos;
		const glm::vec3& v1 = mVertices[mIndices[3 * t + 1]].pos;
		const glm::vec3& v2 = mVertices[mIndices[3 * t + 2]].pos;

		glm::vec3 n = glm::cross(v1 - v0, v2 - v0);
		n = glm::normalize(n);

		float d = -glm::dot(n, v0);

		trianglePlanes[t] = Vec4(n.x, n.y, n.z, d);
	}

	// Compute V:{F}
	// Vertex positions on the structure.
	std::vector<std::vector<uint32_t>> vert2faces(mVertices.size());
	std::vector<uint32_t> vertexArity(mVertices.size(), 0);
	for (uint32_t t = 0; t < (uint32_t)mIndices.size() / 3; ++t) {
		vertexArity[mIndices[3 * t + 0]] += 1;
		vertexArity[mIndices[3 * t + 1]] += 1;
		vertexArity[mIndices[3 * t + 2]] += 1;
	}
	for (uint32_t v = 0; v < (uint32_t)mVertices.size(); ++v) {
		vert2faces[v].reserve(vertexArity[v]);
	}
	for (uint32_t t = 0; t < (uint32_t)mIndices.size() / 3; ++t) {
		vert2faces[mIndices[3 * t + 0]].push_back(t);
		vert2faces[mIndices[3 * t + 1]].push_back(t);
		vert2faces[mIndices[3 * t + 2]].push_back(t);
	}

	while (!tasks.empty()) {

		const OctNodeTask task = std::move(tasks.top());
		tasks.pop();

		float size = octreeSize / static_cast<float>(1 << task.depth);

		for (auto& v : childsVert)  v.clear();

		for (uint32_t dir_slot = 0; dir_slot < 6; ++dir_slot) {
			for (const uint32_t& i : task.sixDirsTris[dir_slot]) {
				const glm::vec3 dir = mVertices[i].pos - task.midCoord;
				uint32_t k =
					((dir.x >= 0.f ? 1 : 0) << 0) +
					((dir.y >= 0.f ? 1 : 0) << 1) +
					((dir.z >= 0.f ? 1 : 0) << 2);
				childsVert[k].vertices[dir_slot].push_back(i);
			}
		}

		// keep generating nodes....
		if (maxDepth  > task.depth) {

			for (uint32_t k = 0; k < 8; ++k) {

				if (!childsVert[k].empty()) {
					glm::vec3 dir = { k & 0b1 ? 1 : -1, k & 0b10 ? 1 : -1, k & 0b100 ? 1 : -1 };
					OctNodeTask newT;
					newT.depth = task.depth + 1;
					newT.midCoord = task.midCoord + 0.25f * size * dir;
					newT.sixDirsTris = childsVert[k];
					tasks.push(std::move(newT));
				}
			}
		}

		// if this depth is one of the LODs, store vertex
		decltype(depthToLod)::const_iterator it = depthToLod.find(task.depth);
		if (it != depthToLod.end()) {
			for (uint32_t dir = 0; dir < 6; ++dir) {
				const std::vector<uint32_t>& vertices = task.sixDirsTris[dir];
				if (vertices.empty()) {
					continue;
				}

				Vertex v;
				v.pos = glm::vec3(0);
				v.normal = glm::vec3(0);

				// Quadric error metric
				if(useQuadricErrorMetric) {
					Mat4 K = Mat4::Zero();

					for (const uint32_t& vId : vertices) {
						for (const uint32_t& f : vert2faces[vId]) {
							const Vec4& P = trianglePlanes[f];

							const Vec4 p = Vec4(P[0], P[1], P[2], P[3] + P.dot(Vec4(task.midCoord.x, task.midCoord.y, task.midCoord.z, 0)));
							K += p * p.transpose();
						}
						v.normal += mVertices[vId].normal;
					}
					v.normal /= vertices.size();

					const Vec4 rhs = Vec4(0, 0, 0, 1.0);
					// Set last row to 0, 0, 0, 1
					K.row(3) = rhs;
					// Compute SVD
					Eigen::JacobiSVD<Mat4> svd(K, Eigen::ComputeFullU | Eigen::ComputeFullV);
					//svd.setThreshold(1e-6);
					const Vec4 res = svd.solve(rhs);

					v.pos = glm::vec3(res[0], res[1], res[2]) + task.midCoord;
				}
				else { // centrod as position
					for (const uint32_t& vId : vertices) {
						v.normal += mVertices[vId].normal;
						v.pos += mVertices[vId].pos;
					}
					v.normal /= vertices.size();
					v.pos /= vertices.size();
				}


				// store new vertex
				const uint32_t vertexIdx = (uint32_t)mLODs[it->second].vertices.size();
				mLODs[it->second].vertices.push_back(v);

				// Store translation table of the new vertex
				for (const uint32_t& i : vertices) {
					old2newVerticesInLod[it->second][i] = vertexIdx;
				}
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

	// Log duration
	const auto end_timer = std::chrono::high_resolution_clock::now();
	typedef std::chrono::duration<double_t> Fsec;

	Fsec dur = end_timer - start_timer;
	std::stringstream ss;
	ss << "Created new Level of details for mesh " << this->getObjectName() << '\n';
	ss << "\tTook " << dur.count() << " seconds\n";
	ss << "\tGenerated " << mLODs.size() << " different LODs\n";

	fc->gc().addNewLog(ss.str());
}