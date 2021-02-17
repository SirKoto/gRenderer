#include "ResourceDictionary.h"

namespace gr
{

ResourceDictionary::ResId ResourceDictionary::addMesh(Mesh&& mesh)
{

	ResId id = getAndUpdateId();

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

Mesh ResourceDictionary::getMesh(ResId id)
{
	std::shared_lock meshLock(mMeshMutex);

	return mMeshes.at(id);
}

void ResourceDictionary::eraseMesh(ResId id)
{
	std::unique_lock meshLock(mMeshMutex);
	mMeshes.erase(id);
}

ResourceDictionary::ResId ResourceDictionary::getAndUpdateId()
{
	std::unique_lock idLock(mNextIdMutex);

	return mNextId++;
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
