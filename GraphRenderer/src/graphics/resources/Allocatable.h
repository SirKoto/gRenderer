#pragma once

#include "../memory/MemoryManager.h"


namespace gr
{
namespace vkg
{

class Allocatable
{
public:
	Allocatable() = default;
	Allocatable(VmaAllocation allocation) : mAllocation(allocation) {}

	void setAllocation(VmaAllocation allocation) { mAllocation = allocation; }

	VmaAllocation getAllocation() const { return mAllocation; }

protected:
	VmaAllocation mAllocation;
};
}
}