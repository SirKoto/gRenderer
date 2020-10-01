#pragma once

#include "../memory/MemoryManager.h"

class Image
{
public:
	void setImage(vk::Image image) { mImage = image; }
	void setAllocation(VmaAllocation allocation) { mAllocation = allocation; }

	vk::Image getVkImage() const { return mImage; }
	VmaAllocation getAllocation() const { return mAllocation; }

protected:
	vk::Image mImage;
	VmaAllocation mAllocation;

	friend class Admin;
};

