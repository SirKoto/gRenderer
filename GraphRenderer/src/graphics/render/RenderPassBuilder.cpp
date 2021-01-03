#include "RenderPassBuilder.h"

namespace gr
{
namespace vkg
{

vk::AttachmentReference RenderPassBuilder::pushColorAttachmentDescription(
	vk::Format tFormat, 
	vk::SampleCountFlagBits tSamples,
	vk::AttachmentLoadOp tLoadOp,
	vk::AttachmentStoreOp tStoreOp,
	vk::ImageLayout tInitialLayout,
	vk::ImageLayout tFinalLayout)
{
	vk::AttachmentReference ref(
		static_cast<uint32_t>(mAttachmentDescriptions.size()), // Reference index
		vk::ImageLayout::eColorAttachmentOptimal	 // Image layout
		);

	mAttachmentDescriptions.emplace_back(
		vk::AttachmentDescription(
			{},			// flags
			tFormat,	// format
			tSamples,	// samples
			tLoadOp,	// load Op
			tStoreOp,	// store Op
			vk::AttachmentLoadOp::eDontCare, // stencil Ops
			vk::AttachmentStoreOp::eDontCare,
			tInitialLayout,
			tFinalLayout
		)
	);

	return ref;
}

vk::AttachmentReference RenderPassBuilder::pushDepthAttachmentDescription(
	vk::Format tFormat,
	vk::SampleCountFlagBits tSamples,
	vk::AttachmentLoadOp tLoadOp,
	vk::AttachmentStoreOp tStoreOp,
	vk::ImageLayout tInitialLayout,
	vk::ImageLayout tFinalLayout)
{
	vk::AttachmentReference ref(
		static_cast<uint32_t>(mAttachmentDescriptions.size()), // Reference index
		vk::ImageLayout::eDepthStencilAttachmentOptimal	 // Image layout
	);

	mAttachmentDescriptions.emplace_back(
		vk::AttachmentDescription(
			{},			// flags
			tFormat,	// format
			tSamples,	// samples
			tLoadOp,	// load Op
			tStoreOp,	// store Op
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			tInitialLayout,
			tFinalLayout
		)
	);

	return ref;
}

uint32_t RenderPassBuilder::pushGraphicsSubpassDescriptionSimple(
	const vk::AttachmentReference* pColorAttachment,
	const vk::AttachmentReference* pDepthStencilAttachment,
	const vk::AttachmentReference* pResolveAttachment)
{
	mSubpassDescriptions.emplace_back(
		vk::SubpassDescription(
			{},									// flags
			vk::PipelineBindPoint::eGraphics,	// Bind point
			0, nullptr,							// no imput attachments
			1, pColorAttachment,				// One color attachment
			pResolveAttachment,					// Associated resolve attachment (may be null)
			pDepthStencilAttachment,			// may also be null
			0, nullptr							// no preserve attachments
	)
	);

	return static_cast<uint32_t>(mSubpassDescriptions.size() - 1);
}

void RenderPassBuilder::pushSubpassDependency(const vk::SubpassDependency& dependency)
{
	mDependencies.push_back(dependency);
}

RenderPass RenderPassBuilder::buildRenderPass(const Context& context) const
{
	RenderPass pass;

	vk::RenderPassCreateInfo info(
		{}, // flags
		static_cast<uint32_t>(mAttachmentDescriptions.size()),
		mAttachmentDescriptions.data(),
		static_cast<uint32_t>(mSubpassDescriptions.size()),
		mSubpassDescriptions.data(),
		static_cast<uint32_t>(mDependencies.size()),	
		mDependencies.data());

	pass.mRenderPass = context.getDevice().createRenderPass(info);

	return pass;
}

}; // namespace vkg
}; // namespace gr