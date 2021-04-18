#pragma once

#include "../graphics/resources/Image2D.h"
#include "IObject.h"

namespace gr
{
// Forward declaration
namespace vkg {
class RenderContext;
}
class FrameContext;
class GlobalContext;

class Texture : public IObject
{
public:

	Texture() = default;
	Texture(FrameContext* fc) : IObject(fc) {}


	bool load(vkg::RenderContext* rc,
		const char* filePath);

	void scheduleDestroy(FrameContext* fc) override final;
	void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override final;

	static constexpr const char* s_getClassName() { return "Texture"; }


protected:

	vkg::Image2D mImage2d;
	std::string mPath;

	// Serialization functions
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(cereal::base_class<IObject>(this));
		archive(GR_SERIALIZE_NVP_MEMBER(mPath));
	}

	GR_SERIALIZE_PRIVATE_MEMBERS
};

} // namespace gr

GR_SERIALIZE_TYPE(gr::Texture)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::Texture)