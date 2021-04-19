#pragma once

#include <string>
#include <stdint.h>

#include <vulkan/vulkan.hpp>

#include "../utils/serialization.h"
#include "ResourcesHeader.h"

namespace gr
{

class FrameContext;
class GlobalContext;

class IObject
{
public:
	IObject() = default;

	virtual ~IObject() = default;

	const std::string& getObjectName() const { return mObjectName; }
	void setObjectName(const std::string& newName) { mObjectName = newName; }

	virtual void scheduleDestroy(FrameContext* fc) = 0;

	struct GuiFeedback {
		ResId selectResource;
	};
	virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) = 0;

	virtual void start(FrameContext* fc) {}

	static constexpr const char* s_getClassName() { return "IObject"; }

private:
	std::string mObjectName;

	uint64_t mFrameLastUpdate = 0;

protected:

	void markUpdated(FrameContext* fc);

	// Serialization functions
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(GR_SERIALIZE_NVP_MEMBER(mObjectName));
	}

	GR_SERIALIZE_PRIVATE_MEMBERS

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