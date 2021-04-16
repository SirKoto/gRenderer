#include "Texture.h"

#include <imgui/imgui.h>

#include "../control/FrameContext.h"
#include "../graphics/RenderContext.h"
#include "../utils/grTools.h"

namespace gr
{



bool Texture::load(vkg::RenderContext* rc, const char* filePath)
{
	uint32_t width, height;
	uint8_t* img;
	tools::loadImageRGBA(filePath, &img, &width, &height);
	if (img == nullptr) {
		return false;
	}

	mPath = filePath;

	vk::DeviceSize imSize = 4 * width * height;

	mImage2d = rc->createTexture2D(
		{ static_cast<vk::DeviceSize>(width),
			static_cast<vk::DeviceSize>(height) }, // extent
		1, vk::SampleCountFlagBits::e1, // mip levels and samples
		vk::Format::eR8G8B8A8Srgb,
		vk::ImageAspectFlagBits::eColor
	);

	rc->getTransferer()->transferToImage(
		*rc, img,	// rc and data ptr
		imSize, mImage2d,		// bytes, Image2D
		vk::ImageSubresourceLayers(
			vk::ImageAspectFlagBits::eColor,
			0, 0, 1 // mip level, base array, layer count
		),
		vk::AccessFlagBits::eShaderRead, // dst Access Mask
		vk::ImageLayout::eShaderReadOnlyOptimal, // dst Image Layout
		vk::PipelineStageFlagBits::eFragmentShader, // dstStage
		true
	);

	tools::freeImage(img);
	return true;
}

void Texture::scheduleDestroy(FrameContext* fc)
{
	fc->scheduleToDestroy(mImage2d);
}


void Texture::renderImGui(FrameContext* fc, GuiFeedback* feedback)
{
	ImGui::TextDisabled("Texture 2D");
	ImGui::Separator();

	ImGui::Text("Image size: %u x %u", mImage2d.getExtent().width, mImage2d.getExtent().height);

	ImGui::Separator();
	ImGui::Text("Path of texture:");
	ImGui::InputText(
		"##Path",
		const_cast<char*>(mPath.c_str()),
		mPath.size(),
		ImGuiInputTextFlags_ReadOnly
	);

}

} // namespace gr