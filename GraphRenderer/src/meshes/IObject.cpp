#include "IObject.h"

#include "../control/FrameContext.h"

void gr::IObject::markUpdated(FrameContext* fc)
{
	mFrameLastUpdate = fc->getFrameCount();
}
