#pragma once

#include "RenderPass.h"

namespace gr
{
namespace vkg
{

class RenderPassBuilder
{
public:

	inline void reserveNumAttachmentDescriptions(uint32_t num);
	inline void reserveNumSubpassDescriptions(uint32_t num);
	inline void reserveNumDependencies(uint32_t num);

	vk::AttachmentReference pushColorAttachmentDescription(
		vk::Format format,
		vk::SampleCountFlagBits samples,
		vk::AttachmentLoadOp loadOp,
		vk::AttachmentStoreOp storeOp,
		vk::ImageLayout initialLayout,
		vk::ImageLayout finalLayout = vk::ImageLayout::eColorAttachmentOptimal
	);

	vk::AttachmentReference pushDepthAttachmentDescription(
		vk::Format format,
		vk::SampleCountFlagBits samples,
		vk::AttachmentLoadOp loadOp,
		vk::AttachmentStoreOp storeOp,
		vk::ImageLayout initialLayout,
		vk::ImageLayout finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
	);

	uint32_t pushGraphicsSubpassDescriptionSimple(
		const vk::AttachmentReference* pColorAttachment,
		const vk::AttachmentReference* pDepthStencilAttachment = nullptr,
		const vk::AttachmentReference* pResolveAttachment = nullptr
	);

	void pushSubpassDependency(const vk::SubpassDependency& dependency);

	RenderPass buildRenderPass(const RenderContext& context) const;

private:
	std::vector<vk::AttachmentDescription> mAttachmentDescriptions;
	std::vector<vk::SubpassDescription> mSubpassDescriptions;
	std::vector<vk::SubpassDependency> mDependencies;
};

// Implementations to try inline....

void RenderPassBuilder::reserveNumAttachmentDescriptions(uint32_t tn)
{
	mAttachmentDescriptions.reserve(tn);
}

void RenderPassBuilder::reserveNumSubpassDescriptions(uint32_t tn)
{
	mSubpassDescriptions.reserve(tn);
}

void RenderPassBuilder::reserveNumDependencies(uint32_t tn)
{
	mDependencies.reserve(tn);
}

}; // namespace vkg
}; // namespace gr