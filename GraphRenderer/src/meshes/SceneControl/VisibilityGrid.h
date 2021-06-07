#pragma once

#include <vector>

#include "../IObject.h"

namespace gr
{

class VisibilityGrid : public IObject
{
public:

	VisibilityGrid();

	virtual void scheduleDestroy(FrameContext* fc) override {}
	virtual void renderImGui(FrameContext* fc, Gui* gui) override;


private:
	std::vector<std::vector<uint8_t>> mWallsGrid;
	uint32_t mResolutionX, mResolutionY;
};

} // namespace gr
