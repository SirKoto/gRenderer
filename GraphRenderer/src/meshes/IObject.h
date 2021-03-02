#pragma once

#include <string>
#include <stdint.h>

#include <vulkan/vulkan.hpp>

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

enum class VertexInputFlags {
	eVertex,
	eColor,
	eNormal,
	eUV,
	COUNT
};

inline std::string to_string(const VertexInputFlags flag) {
	switch (flag)
	{
	case VertexInputFlags::eVertex: return "Vertex";
	case VertexInputFlags::eColor: return "Color";
	case VertexInputFlags::eNormal: return "Normal";
	case VertexInputFlags::eUV: return "UV";
	default: return "invalid (" + vk::toHexString(static_cast<uint32_t>(flag)) + ")";
	}
}

} // namespace gr