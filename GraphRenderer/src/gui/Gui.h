#pragma once

#include "../control/FrameContext.h"

namespace gr {


class Gui
{
public:


	
	void init(GlobalContext* gc);

	void destroy(const vkg::RenderContext& rc);

	void updatePipelineState(vkg::RenderContext* rc,
		vk::RenderPass renderPass,
		uint32_t subpass);

	void updatePreFrame(FrameContext* fc);

	void render(FrameContext* fc, vk::CommandBuffer cmd);

	void addFont(const char* filename);

	void uploadFontObjects(vkg::RenderContext* rc);

	bool appShouldClose() const { return mCloseAppFlag; }

	bool isWireframeRenderModeEnabled() const { return mWireframeModeEnabled; }

	// returns true if there is a mesh pending to open
	bool isMeshToOpen(const char** filePath = nullptr,
		const char** fileName = nullptr) const;
	void setMeshOpened() { mIsMeshToOpen = false; }

protected:

	vk::Pipeline mPipeline;
	vkg::Buffer mIndexBuffer;
	vkg::Buffer mVertexBuffer;
	uint8_t* mIdxPtrMap = nullptr;
	uint8_t* mVertPtrMap = nullptr;
	vkg::Image2D mFontImage;

	vk::DescriptorSetLayout mDescriptorLayout;
	vk::DescriptorSet mDescriptorSet;
	vk::PipelineLayout mPipelineLayout;

	vk::Sampler mTexSampler;

	bool mWireframeModeEnabled = false;

	bool mFilePickerInUse = false;

	bool mCloseAppFlag = false;
	bool mWindowImGuiMetricsOpen = false;
	bool mWindowStyleEditor = false;
	bool mWindowMeshesOpen = false;
	bool mWindowTexturesOpen = false;

	bool mWindowRenameOpen = false;

	std::string mMeshToOpenFilePath;
	std::string mMeshToOpenFileName;
	std::string mRenameString;
	ResourceDictionary::ResId mRenameId;

	bool mIsMeshToOpen = false;



	void drawWindows(FrameContext* fc);
	void drawMainMenuBar(FrameContext* fc);
	void drawStyleWindow(FrameContext* fc);
	void drawFilePicker(FrameContext* fc);
	void drawResourcesWindows(FrameContext* fc);
	void drawRenameWindow(FrameContext* fc);

	// make sure to push id before
	void appendRenameButton(FrameContext* fc, const std::string& name);

	static int s_stringTextCallback(void* data);
};

} // namespace gr

