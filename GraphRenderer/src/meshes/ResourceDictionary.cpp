#include "ResourceDictionary.h"

#include "../control/FrameContext.h"

#include <iostream>

namespace gr
{

ResourceDictionary::ResId ResourceDictionary::addMesh(
	FrameContext* fc, const MeshCreateInfo& createInfo
)
{


	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
	try {
		mesh->load(&fc->rc(), createInfo.filePath);
	}
	catch (std::exception exc) {
		std::cerr << "Exception on loading mesh " << exc.what() << std::endl;
		return 0;
	}

	ResId id = getAndUpdateId();


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
		mMeshes.emplace(id, std::move(mesh));
	}

	return id;
}

void ResourceDictionary::updateMesh(ResId id, Mesh&& mesh)
{
	std::shared_lock meshLock(mMeshMutex);
	*mMeshes.at(id) = std::move(mesh);
}

const Mesh& ResourceDictionary::getMesh(ResId id) const
{
	std::shared_lock meshLock(mMeshMutex);

	return *mMeshes.at(id);
}

void ResourceDictionary::eraseMesh(FrameContext* fc, ResId id)
{
	std::unique_lock meshLock(mMeshMutex);
	std::unordered_map<uint64_t, std::unique_ptr<Mesh>>::iterator it =
		mMeshes.find(id);
	if (it == mMeshes.end()) {
		return;
	}
	it->second->scheduleDestroy(fc);
	mMeshesToFree.push_back(std::move(it->second));
	mMeshes.erase(it);
}

ResourceDictionary::ResId ResourceDictionary::getAndUpdateId()
{
	std::unique_lock idLock(mNextIdMutex);

	return mNextId++;
}

void ResourceDictionary::destroy(GlobalContext* gc)
{
	for (std::pair<const ResId, std::unique_ptr<Mesh>>& it : mMeshes) {
		it.second->destroy(&gc->rc());
	}
	for (std::pair<const ResId, std::unique_ptr<Texture>>& it : mTextures) {
		it.second->destroy(&gc->rc());
	}
}

void ResourceDictionary::flushDataAndFree()
{
	mMeshesToFree.clear();
	mTexturesToFree.clear();
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

ResourceDictionary::ResId ResourceDictionary::addTexture(FrameContext* fc, const TextureCreateInfo& createInfo)
{
	std::unique_ptr<Texture> tex = std::make_unique<Texture>();
	bool loaded = tex->load(&fc->rc(), createInfo.filePath);
	if (!loaded) {
		return 0;
	}

	ResId id = getAndUpdateId();

	// check if the name is unique or create new
	{
		std::unique_lock stringLock(mIdentifierMutex);
		std::string name = createInfo.textureName;
		if (mName2Id.count(name) != 0) {
			name = createUniqueName(name);
		}

		std::pair<decltype(mName2Id)::const_iterator, bool> it =
			mName2Id.emplace(std::move(name), id);
		assert(it.second);
		mId2Name.emplace(id, it.first->first.c_str());
	}

	{
		std::unique_lock texLock(mTexMutex);
		mTextures.emplace(id, std::move(tex));
	}

	return id;
}

void ResourceDictionary::updateTexture(ResId id, Texture&& tex)
{
	std::shared_lock texLock(mTexMutex);
	*mTextures.at(id) = std::move(tex);
}

const Texture& ResourceDictionary::getTexture(ResId id) const
{
	std::shared_lock texLock(mTexMutex);
	return *mTextures.at(id);
}

void ResourceDictionary::eraseTexture(FrameContext* fc, ResId id)
{
	std::unique_lock texLock(mTexMutex);
	std::unordered_map<uint64_t, std::unique_ptr<Texture>>::iterator it =
		mTextures.find(id);
	if (it == mTextures.end()) {
		return;
	}
	it->second->scheduleDestroy(fc);
	mTexturesToFree.push_back(std::move(it->second));
	mTextures.erase(it);
}

ResourceDictionary::ResId ResourceDictionary::getId(const std::string& name) const
{
	std::shared_lock slock(mIdentifierMutex);
	return mName2Id.at(name);
}

std::string ResourceDictionary::getName(const ResId id) const
{
	std::shared_lock slock(mIdentifierMutex);
	return std::string(mId2Name.at(id));
}

bool ResourceDictionary::existsName(const std::string& name) const
{
	std::shared_lock slock(mIdentifierMutex);

	return mName2Id.count(name) != 0;
}

void ResourceDictionary::rename(ResId id, const std::string& newName)
{
	std::unique_lock slock(mIdentifierMutex);

	decltype(mId2Name)::iterator it = mId2Name.find(id);

	mName2Id.erase(it->second);
	std::pair<decltype(mName2Id)::const_iterator, bool> insertIt = 
		mName2Id.emplace(newName, id);
	// assert inserted
	assert(insertIt.second);
	it->second = insertIt.first->first.c_str();

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

std::vector<std::string> ResourceDictionary::getAllTextureNames() const
{
	std::shared_lock lock(mIdentifierMutex);
	std::vector<std::string> res;
	res.reserve(mMeshes.size());

	for (const ResId& id : getAllTextureIds()) {
		res.push_back(mId2Name.at(id));
	}

	return res;
}

std::vector<ResourceDictionary::ResId> ResourceDictionary::getAllMeshesIds() const
{
	std::shared_lock lock(mMeshMutex);
	std::vector<ResId> res;
	res.reserve(mMeshes.size());

	for (const std::pair<const ResId, std::unique_ptr<Mesh>>& it : mMeshes) {
		res.push_back(it.first);
	}

	return res;
}

std::vector<ResourceDictionary::ResId> ResourceDictionary::getAllTextureIds() const
{
	std::shared_lock lock(mTexMutex);
	std::vector<ResId> res;
	res.reserve(mMeshes.size());

	for (const std::pair<const ResId, std::unique_ptr<Texture>>& it : mTextures) {
		res.push_back(it.first);
	}

	return res;
}

} // namespace gr
