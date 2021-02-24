#pragma once

#include <string>
#include <stdint.h>

namespace gr
{

class FrameContext;
class GlobalContext;

class IObject
{
public:

	virtual ~IObject() = default;

	const std::string& getObjectName() const { return mObjectName; }
	void setObjectName(const std::string& newName) { mObjectName = newName; }

	virtual void destroy(GlobalContext* gc) = 0;
	virtual void scheduleDestroy(FrameContext* fc) = 0;
	virtual void renderImGui(FrameContext* fc) = 0;

private:
	std::string mObjectName;
};

} // namespace gr