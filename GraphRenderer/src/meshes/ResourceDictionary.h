#pragma once

#include "ResourcesHeader.h"

#include <unordered_map>
#include <set>
#include <shared_mutex>
#include <memory>
#include <iostream>

#include "../utils/serialization.h"

#include "Mesh.h"
#include "Texture.h"
#include "Sampler.h"
#include "DescriptorSetLayout.h"
#include "Material.h"
#include "Shader.h"
#include "Pipeline.h"
#include "Scene.h"
#include "GameObject.h"



namespace gr
{

class FrameContext;
class GlobalContext;

class ResourceDictionary
{
public:

	
	static constexpr bool CONFIG_USE_DYNAMIC_CAST = true;

	ResourceDictionary& operator=(const ResourceDictionary&) = delete;

	template<typename T>
	ResId allocateObject(FrameContext* fc, std::string objectName, T** outPtr = nullptr);

	template<typename T>
	void get(ResId id, T** object) const;

	// erase will schedule destroy on the item
	void erase(ResId id);

	uint32_t size() const;
	bool empty() const;

	// clear the data structure
	void clear(FrameContext* fc);

	ResId getId(const std::string&  name) const;
	std::string getName(const ResId id) const;
	bool existsName(const std::string& name) const;
	bool exists(const ResId id) const;
	void rename(ResId id, const std::string& newName);

	template <typename T>
	std::vector<ResId> getAllObjectsOfType() const;


	void destroy(FrameContext* gc);
	void flushDataAndFree(FrameContext* fc);

protected:

	std::vector<ResId> mObjectsToFree;
	std::unordered_map<ResId, std::unique_ptr<IObject>> mObjectsDictionary;
	std::array<std::unordered_set<ResId>, ctools::length<ResourceTypesList>()> mObjectsByType;

	std::unordered_map<std::string, ResId> mName2Id;

	ResId mLastIdUsed = ResId(1);

	mutable std::mutex mEraseObjectMutex;
	mutable std::shared_mutex mObjectsMutex;
	mutable std::mutex mNextIdMutex;


	ResId getAndUpdateId();
	// Create new unique name from string. Does not lock!
	std::string createUniqueName(const std::string& string);


	// Serialization functions
	template<class Archive>
	void serialize(Archive& archive)
	{
		std::shared_lock slock(mObjectsMutex);

		archive(GR_SERIALIZE_NVP_MEMBER(mLastIdUsed));
		archive(GR_SERIALIZE_NVP_MEMBER(mName2Id));
		archive(GR_SERIALIZE_NVP_MEMBER(mObjectsByType));
		archive(GR_SERIALIZE_NVP_MEMBER(mObjectsDictionary));
	}

	GR_SERIALIZE_PRIVATE_MEMBERS
};

template<typename T>
ResId ResourceDictionary::allocateObject(
	FrameContext* fc,
	std::string objectName,
	T** outPtr)
{
	constexpr size_t typeIdx = ctools::indexOf<ResourceTypesList, T>();
	static_assert(typeIdx != -1, "Type not added to Dictionary");
	const ResId id = getAndUpdateId();


	// check if the name is unique or create new
	{
		std::unique_lock lock(mObjectsMutex);

		if (mName2Id.count(objectName) != 0) {
			objectName = createUniqueName(objectName);
		}

		std::pair<decltype(mName2Id)::const_iterator, bool> itName =
			mName2Id.emplace(std::move(objectName), id);
		assert(itName.second);
		// Create object and reference
		std::pair<decltype(mObjectsDictionary)::iterator, bool> itObj =
			mObjectsDictionary.emplace(id, std::unique_ptr<IObject>(new T(fc)));
		assert(itObj.second); // assert inserted
		std::pair<std::unordered_set<ResId>::iterator, bool> itObjType =
			mObjectsByType[typeIdx].insert(id);
		assert(itObjType.second);

		itObj.first->second->setObjectName(itName.first->first);

		// return reference
		if (outPtr != nullptr) {
			if (CONFIG_USE_DYNAMIC_CAST) {
				*outPtr = dynamic_cast<T*>(itObj.first->second.get());
				assert(*outPtr != nullptr);
			}
			else {
				*outPtr = reinterpret_cast<T*>(itObj.first->second.get());
			}
		}
	}

	return id;
}

template<typename T>
void gr::ResourceDictionary::get(ResId id, T** object) const
{
	assert(object != nullptr);
	std::shared_lock lock(mObjectsMutex);

	IObject* ptr = mObjectsDictionary.at(id).get();
	if (CONFIG_USE_DYNAMIC_CAST) {
		*object = dynamic_cast<T*>(ptr);
		assert(*object != nullptr);
	}
	else {
		*object = reinterpret_cast<T*>(ptr);
	}
}


template<typename T>
std::vector<gr::ResId> 
gr::ResourceDictionary::getAllObjectsOfType() const
{
	constexpr size_t typeIdx = ctools::indexOf<ResourceTypesList, T>();
	static_assert(typeIdx != -1, "Type not added to Dictionary");
	std::shared_lock lock(mObjectsMutex);

	std::vector<ResId> res;
	res.reserve(mObjectsByType[typeIdx].size());
	for (const ResId& id : mObjectsByType[typeIdx]) {
		res.push_back(id);
	}

	return res;
}

} // namespace gr
