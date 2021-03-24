#include "IObject.h"

#include "../control/FrameContext.h"

void gr::IObject::update(FrameContext* fc)
{
	if (needsUpdate) {
		needsUpdate = false;
		this->updateInternal(fc);
		this->markUpdated(fc);
	}
}

void gr::IObject::markUpdated(FrameContext* fc)
{
	mFrameLastUpdate = fc->getFrameCount();
}

void gr::IObject::signalNeedsLatterUpdate()
{
	needsUpdate = true;
}
