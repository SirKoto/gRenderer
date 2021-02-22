#pragma once

#include "../graphics/resources/Image2D.h"

#include <string>

namespace gr
{
// Forward declaration
namespace vkg {
class RenderContext;
}
class FrameContext;

class Texture
{
public:

	bool load(vkg::RenderContext* rc,
		const char* filePath);

	void scheduleDestroy(FrameContext* fc);
	void destroy(vkg::RenderContext* rc);

protected:

	vkg::Image2D mImage2d;
	std::string mPath;
};

} // namespace gr
