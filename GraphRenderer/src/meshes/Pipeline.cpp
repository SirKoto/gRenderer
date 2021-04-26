#include "Pipeline.h"

#include "../control/FrameContext.h"

namespace gr
{


void Pipeline::scheduleDestroy(FrameContext* fc)
{
	fc->scheduleToDestroy(mPipeline);
	fc->scheduleToDestroy(mPipelineLayout);
}
void Pipeline::renderImGui(FrameContext* fc, Gui* gui)
{

}
} // namespace gr