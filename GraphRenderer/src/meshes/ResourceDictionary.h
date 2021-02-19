#pragma once

#include "Mesh.h"

#include <unordered_map>
#include <shared_mutex>

namespace gr
{

class FrameContext;
class GlobalContext;
class ResourceDictionary
{
public:

	ResourceDictionary& operator=(const ResourceDictionary&) = delete;

	typedef uint64_t ResId;

	// The name of the mesh may be changed if it is not unique
	struct MeshCreateInfo {
		const char* filePath;
		const char* meshName = "";
	};
	// Create a mesh and load it from disk
	ResId addMesh(FrameContext* fc, const MeshCreateInfo& createInfo);
	// Overwrite the mesh with identifier id
	void updateMesh(ResId id, Mesh&& mesh);
	const Mesh& getMesh(ResId id) const;
	void eraseMesh(FrameContext* fc, ResId id);


	void destroy(GlobalContext* gc);

protected:

	std::unordered_map<uint64_t, Mesh> mMeshes;
	
	std::unordered_map<std::string, ResId> mName2Id;

	ResId mNextId = 0;

	mutable std::shared_mutex mMeshMutex;
	mutable std::shared_mutex mName2IdMutex;
	mutable std::mutex mNextIdMutex;


	ResId getAndUpdateId();
	std::string createUniqueName(const std::string& string);


};

} // namespace gr
