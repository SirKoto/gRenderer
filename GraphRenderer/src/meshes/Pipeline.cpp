#include "Pipeline.h"

#include "../control/FrameContext.h"

namespace gr
{
void Pipeline::destroy(GlobalContext* gc)
{
	gc->rc().destroy(mPipeline);
	gc->rc().destroy(mPipelineLayout);
}
void Pipeline::scheduleDestroy(FrameContext* fc)
{
	fc->scheduleToDelete(mPipeline);
	fc->scheduleToDelete(mPipelineLayout);
}
void Pipeline::renderImGui(FrameContext* fc, GuiFeedback* feedback)
{

}
} // namespace gr