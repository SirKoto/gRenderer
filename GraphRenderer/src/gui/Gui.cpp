#include "Gui.h"

#include "../graphics/render/GraphicsPipelineBuilder.h"
#include "../graphics/shaders/VertexInputDescription.h"

#include <imgui/imgui.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <iostream>

namespace gr
{

//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
    Out.Color = aColor;
    Out.UV = aUV;
    gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/
static uint32_t __glsl_shader_vert_spv[] =
{
    0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
    0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
    0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
    0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
    0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
    0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
    0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
    0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
    0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
    0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
    0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
    0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
    0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
    0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
    0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
    0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
    0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
    0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
    0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
    0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
    0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
    0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
    0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
    0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
    0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
    0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
    0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
    0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
    0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
    0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
    0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
    0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
    0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
    0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
    0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
    0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
    0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
    0x0000002d,0x0000002c,0x000100fd,0x00010038
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static uint32_t __glsl_shader_frag_spv[] =
{
    0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
    0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
    0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
    0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
    0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
    0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
    0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
    0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
    0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
    0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
    0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
    0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
    0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
    0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
    0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
    0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
    0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
    0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
    0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
    0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
    0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
    0x00010038
};


constexpr const char * IMPORT_MESH_STRING_KEY = "ImportMeshChooserKeyImGuiFileDialog";
constexpr const char* IMPORT_TEX_STRING_KEY = "ImportTextureChooserKeyImGuiFileDialog";


void Gui::init(GlobalContext* gc)
{
    vkg::RenderContext* rc = &gc->rc();

    // init imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = 
        ImVec2(static_cast<float>(gc->getWindow().getWidth()),
            static_cast<float>(gc->getWindow().getHeigth()));

    ImGui::GetIO().BackendPlatformName = "gr_vulkan_glfw";

    // disable .ini file
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().FontAllowUserScaling = true; // zoom wiht ctrl + mouse wheel 
    // set ImGui input mapping
    {
         ImGuiIO& io = ImGui::GetIO();
#define conv(x) static_cast<uint32_t>(vkg::Window::Input::x)
         io.KeyMap[ImGuiKey_Backspace] = conv(KeyBackspace);
         io.KeyMap[ImGuiKey_Delete] = conv(KeyDelete);
         io.KeyMap[ImGuiKey_Tab] = conv(KeyTab);
         io.KeyMap[ImGuiKey_Home] = conv(KeyHome);
         io.KeyMap[ImGuiKey_End] = conv(KeyEnd);
         io.KeyMap[ImGuiKey_Space] = conv(KeySpace);
         io.KeyMap[ImGuiKey_Enter] = conv(KeyEnter);
         io.KeyMap[ImGuiKey_KeyPadEnter] = conv(KeyEnterKeyPad);
         io.KeyMap[ImGuiKey_Escape] = conv(KeyEscape);

         io.KeyMap[ImGuiKey_A] = conv(KeyA);
         io.KeyMap[ImGuiKey_C] = conv(KeyC);
         io.KeyMap[ImGuiKey_V] = conv(KeyV);
         io.KeyMap[ImGuiKey_X] = conv(KeyX);
         io.KeyMap[ImGuiKey_Y] = conv(KeyY);
         io.KeyMap[ImGuiKey_Z] = conv(KeyZ);

         io.KeyMap[ImGuiKey_UpArrow] = conv(ArrowUp);
         io.KeyMap[ImGuiKey_LeftArrow] = conv(ArrowLeft);
         io.KeyMap[ImGuiKey_DownArrow] = conv(ArrowDown);
         io.KeyMap[ImGuiKey_RightArrow] = conv(ArrowRight);

#undef con
    }

    // font sampler
    {
        vk::SamplerCreateInfo createInfo(
            vk::SamplerCreateFlags{},					// flags
            vk::Filter::eLinear, vk::Filter::eLinear,   // mag&min filter
            vk::SamplerMipmapMode::eLinear,				// mip map
            vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat,		// address mode uvw
            0,										// mip bias
            false                                   // anisotropy
        );

        mTexSampler = rc->getDevice().createSampler(createInfo);
    }

    // Descriptor layout, on bind 0
    {
        // Only one binding, the texture.
        vk::DescriptorSetLayoutBinding texBind(
            0,
            vk::DescriptorType::eCombinedImageSampler,
            1,              // descriptor count
            vk::ShaderStageFlagBits::eFragment,
            &mTexSampler
        );
        vk::DescriptorSetLayoutCreateInfo createInfo(
            {}, 1, &texBind
        );
        mDescriptorLayout = rc->getDevice().createDescriptorSetLayout(createInfo);
    }

    // descriptor set
    {
        rc->allocateDescriptorSet(1, mDescriptorLayout, &mDescriptorSet);
    }

    // pipeline layout
    {
        vk::PushConstantRange pushRange(
            vk::ShaderStageFlagBits::eVertex,   // stage flags
            0,                                  // offset
            4 * sizeof(float)                   // size
        );
        vk::PipelineLayoutCreateInfo createInfo(
            {},                     // flags
            1, &mDescriptorLayout,  // descriptor layouts
            1, &pushRange           // push ranges
        );
        mPipelineLayout = rc->getDevice().createPipelineLayout(createInfo);
    }
}

void Gui::destroy(const vkg::RenderContext& rc)
{
    if (mPipeline) {
        rc.destroy(mPipeline);
        mPipeline = nullptr;
    }
    if (mIdxPtrMap) {
        rc.unmapAllocatable(mIndexBuffer);
        mIdxPtrMap = nullptr;
    }
    if (mVertPtrMap) {
        rc.unmapAllocatable(mVertexBuffer);
        mVertPtrMap = nullptr;
    }
    if (mIndexBuffer) {
        rc.destroy(mIndexBuffer);
    }
    if (mVertexBuffer) {
        rc.destroy(mVertexBuffer);
    }
    if (mFontImage) {
        rc.destroy(mFontImage);
    }
    

    rc.destroy(mPipelineLayout);
    rc.destroy(mDescriptorLayout);
    rc.destroy(mTexSampler);

    ImGui::DestroyContext();
}

void Gui::updatePipelineState(
    vkg::RenderContext* rc,
    vk::RenderPass renderPass,
    uint32_t subpass)
{
    if (mPipeline) {
        rc->destroy(mPipeline);
        mPipeline = nullptr;
    }

    // create shader modules
    vk::ShaderModule fragmentModule, vertexModule;
    {
        vk::ShaderModuleCreateInfo shaderMod(
            {},
            sizeof(__glsl_shader_vert_spv),
            __glsl_shader_vert_spv
        );
        vertexModule = rc->getDevice().createShaderModule(shaderMod);

        shaderMod = vk::ShaderModuleCreateInfo(
            {},
            sizeof(__glsl_shader_frag_spv),
            __glsl_shader_frag_spv
        );
        fragmentModule = rc->getDevice().createShaderModule(shaderMod);
    }


    vkg::GraphicsPipelineBuilder pipBuilder;
    pipBuilder.setShaderStages(vertexModule, fragmentModule);
    {
        vkg::VertexInputDescription vertexIn;
        vertexIn.addBinding(0, sizeof(ImDrawVert))
            .addAttributeFloat(0, 2, offsetof(ImDrawVert, pos))
            .addAttributeFloat(1, 2, offsetof(ImDrawVert, uv))
            .addAttribute8UNORM(2, 4, offsetof(ImDrawVert, col));

        pipBuilder.setVertexBindingDescriptions(vertexIn.getBindingDescription());
        pipBuilder.setVertexAttirbuteDescriptions(vertexIn.getAttributeDescriptions());
    }
    pipBuilder.setMultisampleCount(vk::SampleCountFlagBits::e1);
    pipBuilder.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
    pipBuilder.setColorBlendAttachmentAlphaBlending();
    pipBuilder.setPipelineLayout(mPipelineLayout);
    pipBuilder.addDynamicState(vk::DynamicState::eViewport);
    pipBuilder.addDynamicState(vk::DynamicState::eScissor);
    pipBuilder.setFrontFace(vk::FrontFace::eClockwise);
    mPipeline = pipBuilder.createPipeline(rc->getDevice(), renderPass, subpass);

    rc->destroy(vertexModule);
    rc->destroy(fragmentModule);

}

void Gui::updatePreFrame(FrameContext* fc)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt());

    // update io: dt, screen size
    io.DeltaTime = fc->dtf();
    io.DisplaySize =
        ImVec2(static_cast<float>(fc->getWindow().getWidth()),
            static_cast<float>(fc->getWindow().getHeigth()));

    // set mouse buttons
    io.MouseDown[ImGuiMouseButton_Left] = fc->getWindow().isDown(vkg::Window::Input::MouseLeft);
    io.MouseDown[ImGuiMouseButton_Right] = fc->getWindow().isDown(vkg::Window::Input::MouseRight);

    std::array<double, 2> mousePos;
    fc->getWindow().getMousePosition(&mousePos);
    io.MousePos = ImVec2((float)mousePos[0], (float)mousePos[1]);

    io.MouseWheel += (float)fc->getWindow().getMouseWheelOffset();

    for (uint32_t c : fc->getWindow().getInputCharsUTF()) {
        io.AddInputCharacter(c);
    }

    for (uint32_t k = static_cast<uint32_t>(vkg::Window::Input::KeyA);
        k < static_cast<uint32_t>(vkg::Window::Input::InputCOUNT);
        ++k) {
        io.KeysDown[k] = fc->getWindow().isDown(static_cast<vkg::Window::Input>(k));
    }

    io.KeyCtrl = io.KeysDown[static_cast<uint32_t>(vkg::Window::Input::KeyCtrl)];
    io.KeyAlt = io.KeysDown[static_cast<uint32_t>(vkg::Window::Input::KeyAlt)];
    io.KeyShift = io.KeysDown[static_cast<uint32_t>(vkg::Window::Input::KeyShift)];


    ImGui::NewFrame();

    this->drawWindows(fc);
}


void Gui::render(FrameContext* fc, vk::CommandBuffer cmd)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt());


    ImGui::ShowDemoWindow();

    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();
    assert(drawData->Valid);

    if (drawData->TotalVtxCount > 0) {
        vk::DeviceSize vertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
        vk::DeviceSize indexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);
        // Recreate buffers and map pointers if needed
        if (!mVertexBuffer || mVertexBuffer.getSize() < vertexSize) {
            if (mVertPtrMap) {
                fc->rc().unmapAllocatable(mVertexBuffer);
                mVertPtrMap = nullptr;
            }
            fc->scheduleToDelete(mVertexBuffer);
            mVertexBuffer = fc->rc().createCpuVisibleBuffer(vertexSize,
                vk::BufferUsageFlagBits::eVertexBuffer);
            fc->rc().mapAllocatable(mVertexBuffer, 
                reinterpret_cast<void**>(&mVertPtrMap));
        }
        if (!mIndexBuffer || mIndexBuffer.getSize() < indexSize) {
            if (mIdxPtrMap) {
                fc->rc().unmapAllocatable(mIndexBuffer);
                mIdxPtrMap = nullptr;
            }
            fc->scheduleToDelete(mIndexBuffer);
            mIndexBuffer = fc->rc().createCpuVisibleBuffer(indexSize,
                vk::BufferUsageFlagBits::eIndexBuffer);
            fc->rc().mapAllocatable(mIndexBuffer, 
                reinterpret_cast<void**>(&mIdxPtrMap));
        }

        // upload new vertex and index

        vk::DeviceSize vertOffset = 0;
        vk::DeviceSize idxOffset = 0;
        for (int32_t i = 0; i < drawData->CmdListsCount; ++i)
        {
            const ImDrawList* data = drawData->CmdLists[i];
            std::memcpy(mVertPtrMap + vertOffset,
                data->VtxBuffer.Data, data->VtxBuffer.size_in_bytes());
            std::memcpy(mIdxPtrMap + idxOffset,
                data->IdxBuffer.Data, data->IdxBuffer.size_in_bytes());
            vertOffset += data->VtxBuffer.size_in_bytes();
            idxOffset += data->IdxBuffer.size_in_bytes();
        }

        // Flush range
        std::array<VmaAllocation, 2> allocs =
        { mVertexBuffer.getAllocation(), mIndexBuffer.getAllocation() };
        fc->rc().flushAllocations(allocs.data(), static_cast<uint32_t>(allocs.size()));
    }
    else {
        return;
    }


    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        mPipelineLayout, 0, { mDescriptorSet }, {});
    cmd.bindVertexBuffers(0, { mVertexBuffer.getVkBuffer() }, { 0 });
    cmd.bindIndexBuffer(mIndexBuffer.getVkBuffer(), 0, // offset
        sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);

    {
        vk::Viewport viewport(
            0, 0,       // min x, y
            drawData->DisplaySize.x, drawData->DisplaySize.y, // width height
            0.0f, 1.0f  // min max depth
        );
        cmd.setViewport(0, { viewport });
    }
    {
        std::array<float, 4> pushConstants;
        // scale
        pushConstants[0] = 2.0f / drawData->DisplaySize.x;
        pushConstants[1] = 2.0f / drawData->DisplaySize.y;
        // translation
        pushConstants[2] = -1.0f - drawData->DisplayPos.x * pushConstants[0];
        pushConstants[3] = -1.0f - drawData->DisplayPos.y * pushConstants[1];

        cmd.pushConstants(
            mPipelineLayout, 
            vk::ShaderStageFlags(vk::ShaderStageFlagBits::eVertex),
            0, static_cast<uint32_t>( pushConstants.size() * sizeof(float)),
            pushConstants.data());
    }

    uint32_t vertOffset = 0;
    uint32_t idxOffset = 0;
    for (int32_t i = 0; i < drawData->CmdListsCount; ++i) {
        const ImDrawList* list = drawData->CmdLists[i];
        for (int32_t j = 0; j < list->CmdBuffer.Size; ++j) {
            const ImDrawCmd& imCmd = list->CmdBuffer[j];

            // set scissors
            std::array<int32_t, 2> origin;
            std::array<int32_t, 2> size;
            origin[0] = static_cast<int32_t>(imCmd.ClipRect.x - drawData->DisplayPos.x);
            origin[1] = static_cast<int32_t>(imCmd.ClipRect.y - drawData->DisplayPos.y);
            size[0] = static_cast<int32_t>(imCmd.ClipRect.z - imCmd.ClipRect.x);
            size[1] = static_cast<int32_t>(imCmd.ClipRect.w - imCmd.ClipRect.y);

            // if outside of viewport do not even draw
            if (origin[0] < drawData->DisplaySize.x &&
                origin[1] < drawData->DisplaySize.y &&
                size[0] >= 0.0f &&
                size[1] >= 0.0f) {
                origin[0] = std::max(0, origin[0]);
                origin[1] = std::max(0, origin[1]);

                vk::Rect2D scissor(
                    vk::Offset2D( origin[0], origin[1] ),
                    vk::Extent2D( size[0], size[1] )
                );
                cmd.setScissor(0, { scissor });

                cmd.drawIndexed(imCmd.ElemCount, 1, // elements, instances
                    imCmd.IdxOffset + idxOffset,    // offsets
                    imCmd.VtxOffset + vertOffset, 
                    0                               // first instance
                );
            }
        }
        idxOffset += list->IdxBuffer.Size;
        vertOffset += list->VtxBuffer.Size;
    }
    
}

void Gui::addFont(const char* filename)
{
    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->AddFontFromFileTTF(filename, 18);

    io.Fonts->AddFontFromFileTTF(filename, 16);

    io.Fonts->AddFontFromFileTTF(filename, 21);

}

void Gui::uploadFontObjects(vkg::RenderContext* rc)
{
    if (mFontImage) {
        rc->destroy(mFontImage);
    }

    ImGuiIO& io = ImGui::GetIO();
    uint8_t* pix;
    int32_t w, h;
    io.Fonts->GetTexDataAsRGBA32(&pix, &w, &h);

    mFontImage = rc->createTexture2D(
        vk::Extent2D(w, h),     // extent
        1, vk::SampleCountFlagBits::e1, // mip and samples
        vk::Format::eR8G8B8A8Unorm
    );

    // Update the descriptor set
    {
        vk::DescriptorImageInfo imageDesc(
            mTexSampler, mFontImage.getVkImageview(),
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
        vk::WriteDescriptorSet writeDesc(
            mDescriptorSet, 0,  // dst set and binding
            0, 1,               // dst array element and count
            vk::DescriptorType::eCombinedImageSampler,
            &imageDesc
        );
        rc->getDevice().updateDescriptorSets({ writeDesc }, {});
    }

    // Schedule copy to texture
    rc->getTransferer()->transferToImage(*rc, pix, w * h * 4,
        mFontImage,
        vk::ImageSubresourceLayers(
            vk::ImageAspectFlagBits::eColor,
            0, 0, 1 // mip level, base array, layer count
        ),
        vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eFragmentShader,
        true);

    // the identifier as to be stored
    io.Fonts->TexID = mFontImage.getVkImage();


}


void Gui::drawWindows(FrameContext* fc)
{
    drawMainMenuBar(fc);

    if (mWindowImGuiMetricsOpen) {
        ImGui::ShowMetricsWindow(&this->mWindowImGuiMetricsOpen);
    }

    if (mWindowStyleEditor) {
        ImGui::Begin("Style Editor", &this->mWindowStyleEditor);
        drawStyleWindow(fc);
        ImGui::End();
    }

    drawResourcesWindows(fc);
    //drawRenameWindow(fc);

    drawFilePicker(fc);
}

// template loop to fill with names of used classes
template<size_t N> 
void fill(::std::array<const char*, cexprUtils::length< ResourceDictionary::ResourceTypesList>()>& arr) {
    using RTL = typename ResourceDictionary::ResourceTypesList;
    arr[N - 1] = typeid(cexprUtils::TypeAt<RTL, N - 1>::type).name();
    fill<N - 1>(arr);
}
template<>
void fill<0>(::std::array<const char*, cexprUtils::length< ResourceDictionary::ResourceTypesList>()>& arr)
{ }



void Gui::drawMainMenuBar(FrameContext* fc)
{
    if (ImGui::BeginMainMenuBar()) {

        ImGui::PushID("MainMenu");

        if (ImGui::BeginMenu("File")) {

            if (ImGui::MenuItem("Import mesh", nullptr,
                nullptr, !mFilePickerInUse)) {

                mFilePickerInUse = true;
                // open dialog
                ImGuiFileDialog::Instance()->OpenDialog(
                    IMPORT_MESH_STRING_KEY,
                    "Choose mesh to load",
                    ".obj", // filter
                    "." // directory
                );

            }
            if (ImGui::MenuItem("Import image", nullptr,
                nullptr, !mFilePickerInUse)) {

                mFilePickerInUse = true;
                // open dialog
                ImGuiFileDialog::Instance()->OpenDialog(
                    IMPORT_TEX_STRING_KEY,
                    "Choose image to load",
                    ".png", // filter
                    "." // directory
                );

            }

            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                mCloseAppFlag = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Render")) {
            std::array<const char*, 2> sel =
            { "Color", "Wireframe" };

            if (ImGui::BeginCombo("Render type###Selector", sel[mWireframeModeEnabled])) {
                if(ImGui::Selectable(sel[0], !mWireframeModeEnabled)) {
                    mWireframeModeEnabled = false;
                }
                if(ImGui::Selectable(sel[1], mWireframeModeEnabled)) {
                    mWireframeModeEnabled = true;
                }
                ImGui::EndCombo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Resources")) {
            ImGui::PushID("ResourcesMenu");
            //ImGui::MenuItem("Meshes", nullptr, &this->mWindowMeshesOpen);
            //ImGui::MenuItem("Textures", nullptr, &this->mWindowTexturesOpen);
            using RTL = typename ResourceDictionary::ResourceTypesList;
            ::std::array<const char*, cexprUtils::length<RTL>()> names = {};
            fill< cexprUtils::length< ResourceDictionary::ResourceTypesList>()>(names);
            for (size_t i = 0; i < cexprUtils::length<RTL>(); ++i) {
                ImGui::MenuItem(names[i],
                    nullptr,
                    this->mWindowResourceOpen.data() + i);
            }
            ImGui::PopID();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Metrics", nullptr, &this->mWindowImGuiMetricsOpen);
            ImGui::MenuItem("Style", nullptr, &this->mWindowStyleEditor);

            ImGui::EndMenu();
        }


        ImGui::PopID();
        ImGui::EndMainMenuBar();
    }
}

void Gui::drawStyleWindow(FrameContext* fc)
{
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font_current = ImGui::GetFont();
    if (ImGui::BeginCombo("Font##Selector", font_current->GetDebugName())) {
        for (int i = 0; i < io.Fonts->Fonts.Size; i++)
        {
            ImFont* font = io.Fonts->Fonts[i];
            ImGui::PushID((void*)font);
            if (ImGui::Selectable(font->GetDebugName(), font == font_current))
                io.FontDefault = font;
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
}

void Gui::drawFilePicker(FrameContext* fc)
{

    if (ImGuiFileDialog::Instance()->Display(
        IMPORT_MESH_STRING_KEY, // Key
        ImGuiWindowFlags_NoCollapse,
        ImVec2(0, 20 * ImGui::GetFontSize()) // minSize
    )) {
        assert(mFilePickerInUse);
        mFilePickerInUse = false;
        // action if OK
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::map<std::string, std::string> map = 
                ImGuiFileDialog::Instance()->GetSelection();
            assert(map.size() == 1);

            Mesh* mesh;
            fc->gc().getDict().allocateObject(
                map.begin()->first,
                &mesh
            );
            mesh->load(&fc->rc(), map.begin()->second.c_str());
        }

        // close
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(
        IMPORT_TEX_STRING_KEY, // Key
        ImGuiWindowFlags_NoCollapse,
        ImVec2(0, 20 * ImGui::GetFontSize()) // minSize
    )) {
        assert(mFilePickerInUse);
        mFilePickerInUse = false;
        // action if OK
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::map<std::string, std::string> map =
                ImGuiFileDialog::Instance()->GetSelection();
            assert(map.size() == 1);

            Texture* tex;
            fc->gc().getDict().allocateObject(
                map.begin()->first,
                &tex
            );
            tex->load(&fc->rc(), map.begin()->second.c_str());
        }

        // close
        ImGuiFileDialog::Instance()->Close();
    }
}

void Gui::drawResourcesWindows(FrameContext* fc)
{
    ImGui::PushID("Resources");
    
    drawResourcesWindows_t<cexprUtils::length<ResourceDictionary::ResourceTypesList>()>(fc);

    ImGui::PopID();
}

void Gui::appendRenamePopupItem(FrameContext* fc, const std::string& name)
{
    /*if (ImGui::Button("Rename")) {
        mWindowRenameOpen = true;
        mRenameString = name;
        mRenameId = fc->gc().getDict().getId(name);
    }*/
    if (ImGui::BeginPopupContextItem()) {

        if (mRenameId != fc->gc().getDict().getId(name)) {
            mRenameString = name;
            mRenameId = fc->gc().getDict().getId(name);
        }
        const bool canRenamePre = mRenameString == fc->gc().getDict().getName(mRenameId);

        ImGui::Text("Rename:");
        if (!canRenamePre) {
            ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(255, 10, 10));
        }

        if (ImGui::InputText(
            "####RenameName",
            const_cast<char*>(mRenameString.c_str()),
            mRenameString.capacity() + 1,
            ImGuiInputTextFlags_CallbackResize,
            reinterpret_cast<ImGuiInputTextCallback>(s_stringTextCallback),
            this)) {
            // read again after callback
            const bool canRename = !fc->gc().getDict().existsName(mRenameString);

            if (canRename) {
                fc->gc().getDict().rename(mRenameId, mRenameString);
            }
        }

        if (!canRenamePre) {
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
        helpMarker("- Press enter to close window\n- In red if the name cannot be used");
        if (ImGui::Button("Close") ||
            fc->gc().getWindow().isDown(vkg::Window::Input::KeyEnter) ||
            fc->gc().getWindow().isDown(vkg::Window::Input::KeyEnterKeyPad)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

int Gui::s_stringTextCallback(void* data_) {
    ImGuiInputTextCallbackData* data = reinterpret_cast<ImGuiInputTextCallbackData*>(data_);

    Gui* gui = reinterpret_cast<Gui*>(data->UserData);

    assert(data->Buf == gui->mRenameString.c_str());
    gui->mRenameString.resize(data->BufTextLen);
    data->Buf = const_cast<char*>(gui->mRenameString.c_str());

    return 0;
}


void Gui::helpMarker(const char* text)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

template<size_t N>
inline void Gui::drawResourcesWindows_t(FrameContext* fc)
{
    using Type = typename cexprUtils::TypeAt<ResourceDictionary::ResourceTypesList, N - 1>::type;

    if (mWindowResourceOpen[N - 1]) {

        ImGui::SetNextWindowSizeConstraints(
            ImVec2(10 * ImGui::GetFontSize(), 5 * ImGui::GetFontSize()), // minSize
            ImVec2(FLT_MAX, FLT_MAX) // maxSize
        );

        const char* windowName = typeid(Type).name();
        if (ImGui::Begin(windowName, &mWindowResourceOpen[N - 1])) {
            ImGui::PushID(windowName);

            for (const ResourceDictionary::ResId& id :
                fc->gc().getDict().getAllObjectsOfType<Type>()) {

                std::string name = fc->gc().getDict().getName(id);
                std::string label = name + "###" + std::to_string(id);
                if (ImGui::Button(label.c_str())) {
                    //ImGui::PushID(name.c_str());


                    //ImGui::PopID();
                }
                appendRenamePopupItem(fc, name);

            }

            ImGui::PopID();
        }


        ImGui::End();
    }

    drawResourcesWindows_t<N - 1>(fc);
}

template<>
inline void Gui::drawResourcesWindows_t<0>(FrameContext* fc)
{ }


} // namespace gr