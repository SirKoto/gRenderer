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
	bool mWindowInspectorOpen = true;
	bool mWindowSceneOpen = true;

	std::array<bool, ctools::length<ResourceTypesList>()>
		mWindowResourceOpen;

	std::string mRenameString;
	ResId mRenameId;
	ResId mInspectorResourceId;


	void drawWindows(FrameContext* fc);
	void drawMainMenuBar(FrameContext* fc);
	void drawStyleWindow(FrameContext* fc);
	void drawFilePicker(FrameContext* fc);
	void drawResourcesWindows(FrameContext* fc);
	void drawInspectorWindow(FrameContext* fc);
	void drawSceneWindow(FrameContext* fc);

	void helpMarker(const char* text);

	// make sure to push id before
	// returns true if need to reload resId and check for existance
	bool appendRenamePopupItem(FrameContext* fc, const std::string& name);

	static int s_stringTextCallback(void* data);

private:
	template<size_t N>
	void drawResourcesWindows_t(FrameContext* fc);
};

} // namespace gr

