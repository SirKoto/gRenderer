#include "ResourceDictionary.h"

#include "../control/FrameContext.h"

#include <iostream>

namespace gr
{

ResId ResourceDictionary::getAndUpdateId()
{
	std::unique_lock idLock(mNextIdMutex);

	mNextId.value += 1;

	return mNextId;
}

void ResourceDictionary::destroy(FrameContext* fc)
{
	for (std::pair<const ResId, std::unique_ptr<IObject>>& it : mObjectsDictionary) {
		it.second->scheduleDestroy(fc);
	}
}

void ResourceDictionary::flushDataAndFree(FrameContext* fc)
{
	for (const ResId& id : mObjectsToFree) {
		for (std::set<ResId>& s : mObjectsByType) {
			s.erase(id);
		}
		decltype(mObjectsDictionary)::iterator it = mObjectsDictionary.find(id);
		it->second->scheduleDestroy(fc);
		mObjectsDictionary.erase(it);
	}

	mObjectsToFree.clear();
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

void ResourceDictionary::erase(ResId id)
{
	std::unique_lock lock(mEraseObjectMutex);
	mObjectsToFree.push_back(id);
}

ResId ResourceDictionary::getId(const std::string& name) const
{
	std::shared_lock slock(mObjectsMutex);
	return mName2Id.at(name);
}

std::string ResourceDictionary::getName(const ResId id) const
{
	std::shared_lock slock(mObjectsMutex);
	return mObjectsDictionary.at(id)->getObjectName();
}

bool ResourceDictionary::existsName(const std::string& name) const
{
	std::shared_lock slock(mObjectsMutex);

	return mName2Id.count(name) != 0;
}

bool ResourceDictionary::exists(const ResId id) const
{
	std::shared_lock slock(mObjectsMutex);
	return mObjectsDictionary.count(id) != 0;
}

void ResourceDictionary::rename(ResId id, const std::string& newName)
{
	std::unique_lock slock(mObjectsMutex);

	// assert that the new name is unique
	assert(mName2Id.count(newName) == 0);

	decltype(mObjectsDictionary)::iterator it = mObjectsDictionary.find(id);
	
	mName2Id.erase(it->second->getObjectName());
	std::pair<decltype(mName2Id)::const_iterator, bool> insertIt = 
		mName2Id.emplace(newName, id);
	// assert inserted
	assert(insertIt.second);
	it->second->setObjectName(newName);
}

} // namespace gr
