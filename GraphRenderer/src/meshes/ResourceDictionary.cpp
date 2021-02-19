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
	mesh.load(&fc->rc(), createInfo.filePath);

	// check if the name is unique or create new
	{
		std::unique_lock stringLock(mIdentifierMutex);
		std::string name = createInfo.meshName;
		if (mName2Id.count(name) != 0) {
			name = createUniqueName(name);
		}

		std::pair<decltype(mName2Id)::const_iterator, bool> it =
			mName2Id.emplace(std::move(name), id);
		assert(it.second);
		mId2Name.emplace(id, it.first->first.c_str());
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

ResourceDictionary::ResId ResourceDictionary::getId(const std::string name) const
{
	std::shared_lock slock(mIdentifierMutex);
	return mName2Id.at(name);
}

std::string ResourceDictionary::getName(const ResId id) const
{
	std::shared_lock slock(mIdentifierMutex);
	return std::string(mId2Name.at(id));
}

std::vector<std::string> ResourceDictionary::getAllMeshesNames() const
{
	std::shared_lock lock(mIdentifierMutex);
	std::vector<std::string> res;
	res.reserve(mMeshes.size());

	for (const ResId& id : getAllMeshesIds()) {
		res.push_back(mId2Name.at(id));
	}

	return res;
}

std::vector<ResourceDictionary::ResId> ResourceDictionary::getAllMeshesIds() const
{
	std::shared_lock lock(mMeshMutex);
	std::vector<ResId> res;
	res.reserve(mMeshes.size());

	for (const std::pair<const ResId, Mesh>& it : mMeshes) {
		res.push_back(it.first);
	}

	return res;
}

} // namespace gr
