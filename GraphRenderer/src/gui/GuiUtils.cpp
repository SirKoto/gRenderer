#include "GuiUtils.h"

#include <imgui/imgui.h>

namespace gr
{
namespace gui
{
void helpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
bool isKeyboardCaptured()
{
    return ImGui::GetIO().WantCaptureKeyboard;
}
bool isMouseCaptured()
{
    return ImGui::GetIO().WantCaptureMouse;
}
bool isInputCaptured()
{
    return isKeyboardCaptured() || isMouseCaptured();
}
} // namespace gui
} // namespace gr