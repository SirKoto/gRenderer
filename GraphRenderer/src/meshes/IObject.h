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

	static constexpr const char* s_getClassName() { return "IObject"; }

private:
	std::string mObjectName;

	uint64_t mFrameLastUpdate = 0;

protected:

	void markUpdated(FrameContext* fc);

};

} // namespace gr