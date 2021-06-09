#include "Logger.h"

#include <imgui/imgui.h>

namespace gr {
Logger::Logger()
{
	this->clear();
}
void Logger::clear()
{
	mOffsets.clear();
	mBuffer.clear();
	mOffsets.push_back(0);
}

void Logger::addLog(std::string log)
{
	log.push_back('\n');
	std::unique_lock lock(mMutex);
	size_t oldSize = mBuffer.size();
	mBuffer.insert(mBuffer.end(), log.begin(), log.end());
	for (size_t i = oldSize; i < mBuffer.size(); ++i) {
		if (mBuffer[i] == '\n') {
			mOffsets.push_back(i + 1);
		}
	}
}

void Logger::drawImGui()
{
	bool clear = ImGui::Button("Clear");
	ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	if (clear) {
		this->clear();
	}
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	if (!mBuffer.empty()) {
		const char* buf = mBuffer.data();
		const char* buf_end = mBuffer.data() + mBuffer.size();

		ImGuiListClipper clipper;
		clipper.Begin((int)mOffsets.size());
		while (clipper.Step())
		{
			for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
			{
				const char* line_start = buf + mOffsets[line_no];
				const char* line_end = (line_no + 1 < mOffsets.size()) ? (buf + mOffsets[line_no + 1] - 1) : buf_end;
				ImGui::TextUnformatted(line_start, line_end);
			}
		}
		clipper.End();
	}
	ImGui::PopStyleVar();

	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
}

}