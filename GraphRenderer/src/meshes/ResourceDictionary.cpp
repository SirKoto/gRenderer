#include "ResourceDictionary.h"

#include "../control/FrameContext.h"

namespace gr
{

ResourceDictionary::ResId ResourceDictionary::addMesh(
	FrameContext* fc, const MeshCreateInfo& createInfo
)
{

	ResId id = getAndUpdateId();

	Mesh mesh;
	mesh.load(&fc->rc(), createInfo.filePath, createInfo.meshName);

	// check if the name is unique or create new
	{
		std::unique_lock stringLock(mName2IdMutex);
		if (mName2Id.count(mesh.getName()) != 0) {
			mesh.setName(createUniqueName(mesh.getName()));
		}

		mName2Id.emplace(mesh.getName(), id);
	}

	{
		std::unique_lock meshLock(mMeshMutex);
		mMeshes.emplace(id, mesh);
	}

	return id;
}

void ResourceDictionary::updateMesh(ResId id, Mesh&& mesh)
{
	std::unique_lock meshLock(mMeshMutex);
	mMeshes.at(id) = mesh;
}

const Mesh& ResourceDictionary::getMesh(ResId id) const
{
	std::shared_lock meshLock(mMeshMutex);

	return mMeshes.at(id);
}

void ResourceDictionary::eraseMesh(FrameContext* fc, ResId id)
{
	std::unique_lock meshLock(mMeshMutex);
	std::unordered_map<uint64_t, Mesh>::iterator it =
		mMeshes.find(id);
	if (it == mMeshes.end()) {
		throw std::runtime_error("Error! Trying to erase mesh with non existing id " + id);
	}
	it->second.scheduleDestroy(fc);
	mMeshes.erase(it);
}

ResourceDictionary::ResId ResourceDictionary::getAndUpdateId()
{
	std::unique_lock idLock(mNextIdMutex);

	return mNextId++;
}

void ResourceDictionary::destroy(GlobalContext* gc)
{
	for (std::pair<const ResId, Mesh>& it : mMeshes) {
		it.second.destroy(&gc->rc());
	}
}

std::string ResourceDictionary::createUniqueName(const std::string& string)
{
	std::string newName;
	int i = 0;
	do {
		newName = string + '_' + std::to_string(i++);
	} while (mName2Id.count(newName) != 0);
	return newName;
}

} // namespace gr
