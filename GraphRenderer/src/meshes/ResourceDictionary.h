#pragma once

#include "Mesh.h"
#include "Texture.h"

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
	// Overwrite the mesh with identifier id.
	void updateMesh(ResId id, Mesh&& mesh);
	const Mesh& getMesh(ResId id) const;
	void eraseMesh(FrameContext* fc, ResId id);

	struct TextureCreateInfo {
		const char* filePath;
		const char* textureName = "";
	};
	ResId addTexture(FrameContext* fc, const TextureCreateInfo& createInfo);
	void updateTexture(ResId id, Texture&& tex);
	const Texture& getTexture(ResId id) const;
	void eraseTexture(FrameContext* fc, ResId id);

	ResId getId(const std::string&  name) const;
	std::string getName(const ResId id) const;
	bool existsName(const std::string& name) const;
	void rename(ResId id, const std::string& newName);

	std::vector<std::string> getAllMeshesNames() const;
	std::vector<std::string> getAllTextureNames() const;

	std::vector<ResId> getAllMeshesIds() const;
	std::vector<ResId> getAllTextureIds() const;


	void destroy(GlobalContext* gc);

protected:

	std::unordered_map<ResId, Mesh> mMeshes;
	std::unordered_map<ResId, Texture> mTextures;

	
	std::unordered_map<std::string, ResId> mName2Id;
	std::unordered_map<ResId, const char*> mId2Name;


	ResId mNextId = 1;

	mutable std::shared_mutex mMeshMutex, mTexMutex;
	mutable std::shared_mutex mIdentifierMutex;
	mutable std::mutex mNextIdMutex;


	ResId getAndUpdateId();
	std::string createUniqueName(const std::string& string);


};

} // namespace gr
