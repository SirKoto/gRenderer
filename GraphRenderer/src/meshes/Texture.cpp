#include "Texture.h"

#include <imgui/imgui.h>
#include <filesystem>

#include "../control/FrameContext.h"
#include "../graphics/RenderContext.h"
#include "../utils/grTools.h"

namespace gr
{



bool Texture::load(FrameContext* fc, const char* filePath)
{

	std::filesystem::path path(filePath);
	mPath = std::filesystem::relative(path, fc->gc().getProjectPath()).string();

	vkg::RenderContext* rc = &fc->rc();

	uint32_t width, height;
	uint8_t* img;
	tools::loadImageRGBA(mPath.c_str(), &img, &width, &height);
	if (img == nullptr) {
		return false;
	}

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


void Texture::renderImGui(FrameContext* fc, Gui* gui)
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