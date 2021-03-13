#include "Shader.h"

#include "../control/FrameContext.h"
#include "../utils/grTools.h"

#include <imgui/imgui.h>
#include <spirv_cross/spirv_glsl.hpp>

namespace gr {
void Shader::destroy(GlobalContext* gc)
{
	gc->rc().destroy(mShaderModule);
	mShaderModule = nullptr;
}

void Shader::scheduleDestroy(FrameContext* fc)
{
	this->destroy(&fc->gc());
}

void Shader::renderImGui(FrameContext* fc, GuiFeedback* feedback)
{
	ImGui::TextDisabled("Shader");
	ImGui::Separator();

	if (ImGui::BeginTabBar("##Tab bar")) {

		if (ImGui::BeginTabItem("Description")) {
			constexpr std::array<vk::ShaderStageFlagBits, 2> stages = {
				vk::ShaderStageFlagBits::eVertex,
				vk::ShaderStageFlagBits::eFragment
			};

			if (ImGui::BeginCombo("Shader Stage", vk::to_string(mStage).c_str())) {
				for (int32_t i = 0; i < stages.size(); ++i) {
					if (ImGui::Selectable(
						vk::to_string(stages[i]).c_str(),
						mStage == stages[i])) {
						mStage = stages[i];
					}

					if (mStage == stages[i]) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();

			// input and output locations
			if (!mInLocations.empty()) {
				ImGui::Text("Input locations:");
				if (ImGui::BeginTable("Input locations", 
					mStage == vk::ShaderStageFlagBits::eVertex ? 5 : 4,
					ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
					ImGui::TableSetupColumn("Location");
					ImGui::TableSetupColumn("Name");
					ImGui::TableSetupColumn("Format");
					ImGui::TableSetupColumn("VecSize");
					if (mStage == vk::ShaderStageFlagBits::eVertex) {
						ImGui::TableSetupColumn("Vertex Input");
					}
					ImGui::TableHeadersRow();

					for (std::pair<const uint32_t, InLocationInfo>& loc : mInLocations) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text(std::to_string(loc.first).c_str()); ImGui::TableNextColumn();
						ImGui::Text(loc.second.name.c_str()); ImGui::TableNextColumn();
						ImGui::Text("Float"); ImGui::TableNextColumn();
						ImGui::Text(std::to_string(loc.second.vecSize).c_str());
						if (mStage == vk::ShaderStageFlagBits::eVertex) {
							ImGui::TableNextColumn();
							float w = ImGui::CalcItemWidth();
							// selector to set vertex input only on vertex shader
							std::string label = "##VertexAssembly" + std::to_string(loc.first);
							if (ImGui::BeginCombo(label.c_str(), gr::to_string(loc.second.inputFlags).c_str())) {
								for (int32_t i = 0; i < static_cast<uint32_t>(VertexInputFlags::COUNT); ++i) {
									if (ImGui::Selectable(
										gr::to_string(static_cast<VertexInputFlags>(i)).c_str(),
										loc.second.inputFlags == static_cast<VertexInputFlags>(i))) {
										loc.second.inputFlags = static_cast<VertexInputFlags>(i);
									}
								}
								ImGui::EndCombo();
							}
						}
					}

					ImGui::EndTable();
				}
			}
			if (!mOutLocations.empty()) {
				ImGui::Text("Output locations:");
				if (ImGui::BeginTable("Output locations", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
					ImGui::TableSetupColumn("Location");
					ImGui::TableSetupColumn("Name");
					ImGui::TableSetupColumn("Format");
					ImGui::TableSetupColumn("VecSize");
					ImGui::TableHeadersRow();

					for (const std::pair<const uint32_t, LocationInfo>& loc : mOutLocations) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text(std::to_string(loc.first).c_str()); ImGui::TableNextColumn();
						ImGui::Text(loc.second.name.c_str()); ImGui::TableNextColumn();
						ImGui::Text("Float"); ImGui::TableNextColumn();
						ImGui::Text(std::to_string(loc.second.vecSize).c_str());
					}

					ImGui::EndTable();
				}
			}

			// uniform buffers
			if (!mDescriptorSets.empty()) {
				ImGui::Text("Uniforms:");
				if (ImGui::BeginTable("Uniforms Table",
					static_cast<uint32_t>(BindingInfo::cols.size()), ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
					BindingInfo::drawTableHeader();

					for (auto& set : mDescriptorSets) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						const std::string setName = "Set " + std::to_string(set.first);
						bool open = ImGui::TreeNodeEx(setName.c_str(), ImGuiTreeNodeFlags_None);
						if (open) {
							for (auto& bind : set.second) {
								bind.second->drawTableRow(bind.first);
							}
							ImGui::TreePop();
						}
					}


					ImGui::EndTable();
				}
			}

			ImGui::Separator();

			ImGui::Text("Path of shader:");
			ImGui::InputText(
				"##Path",
				const_cast<char*>(mPath.c_str()),
				mPath.size(),
				ImGuiInputTextFlags_ReadOnly
			);

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Code")) {
			ImGui::BeginChild("scrolling", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

			ImGui::TextUnformatted(mGLSLCode.c_str());

			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

}

std::unique_ptr<Shader::BindingInfo> loadBinding(
	const spirv_cross::SPIRType& type,
	const spirv_cross::CompilerGLSL& cmp) {
	const uint32_t count = static_cast<uint32_t>(type.member_types.size());

	std::unique_ptr<Shader::BindingInfo> bindInfo;
	if (type.basetype == spirv_cross::SPIRType::BaseType::Struct) {
		bindInfo = std::make_unique<Shader::StructInfo>();
		bindInfo->name = cmp.get_name(type.self);
	}
	else if (type.basetype == spirv_cross::SPIRType::BaseType::Float) {
		bindInfo = std::make_unique<Shader::BasicInfo>();
	}
	else if (type.basetype == spirv_cross::SPIRType::BaseType::SampledImage) {
		bindInfo = std::make_unique<Shader::SampledImgInfo>();
		bindInfo->name = cmp.get_name(type.self);
	}
	else {
		throw std::runtime_error("Shader Binding type not implemented!");
	}

	for (uint32_t i = 0; i < count; ++i) {
		std::unique_ptr<Shader::BindingInfo> child = loadBinding(cmp.get_type(type.member_types[i]), cmp);
		child->name = cmp.get_member_name(type.self, i);
		child->setOffset(cmp.type_struct_member_offset(type, i));
		child->bytes = static_cast<uint32_t>(cmp.get_declared_struct_member_size(type, i));
		bindInfo->bytes += child->bytes;
		bindInfo->addChild(std::move(child));
	}

	return bindInfo;
}


void Shader::load(FrameContext* fc, const char* filePath)
{
	if (mShaderModule) {
		this->destroy(&fc->gc());
	}

	mPath = filePath;

	std::vector<uint32_t> spir;
	fc->rc().createShaderModule(filePath, &this->mShaderModule, &spir);

	if (std::strstr(filePath, "vert")) {
		mStage = vk::ShaderStageFlagBits::eVertex;
	}
	else if (std::strstr(filePath, "frag")) {
		mStage = vk::ShaderStageFlagBits::eFragment;
	}

	spirv_cross::CompilerGLSL cmp(std::move(spir));

	spirv_cross::ShaderResources resources = cmp.get_shader_resources();
	// Get inputs
	for (spirv_cross::Resource& res : resources.stage_inputs) {
		unsigned location = cmp.get_decoration(res.id, spv::DecorationLocation);
		spirv_cross::SPIRType type = cmp.get_type(res.type_id);

		if (type.basetype != spirv_cross::SPIRType::BaseType::Float) {
			throw std::runtime_error("Error shader! type is not float");
		}

		// if already inserted
		if (!mInLocations.insert({
			location,
			InLocationInfo{Type::eFloat, type.vecsize, res.name} })
			.second) {
			throw std::runtime_error("Error shader! The location is already used");
		}
	}

	//outputs
	for (spirv_cross::Resource& res : resources.stage_outputs) {
		uint32_t location = cmp.get_decoration(res.id, spv::DecorationLocation);
		const spirv_cross::SPIRType& type = cmp.get_type(res.type_id);

		if (type.basetype != spirv_cross::SPIRType::BaseType::Float) {
			throw std::runtime_error("Error shader! type is not float");
		}

		// if already inserted
		if (!mOutLocations.insert({
			location,
			LocationInfo{Type::eFloat, type.vecsize, res.name} })
			.second) {
			throw std::runtime_error("Error shader! The location is already used");
		}
	}

	// uniform buffers
	for (spirv_cross::Resource& res : resources.uniform_buffers) {
		uint32_t set = cmp.get_decoration(res.id, spv::DecorationDescriptorSet);
		uint32_t bind = cmp.get_decoration(res.id, spv::DecorationBinding);

		const spirv_cross::SPIRType& type = cmp.get_type(res.base_type_id);

		mDescriptorSets[set].emplace(bind, 
			loadBinding(type, cmp));
	}

	// sampled images
	for (spirv_cross::Resource& res : resources.sampled_images) {
		uint32_t set = cmp.get_decoration(res.id, spv::DecorationDescriptorSet);
		uint32_t bind = cmp.get_decoration(res.id, spv::DecorationBinding);
		const spirv_cross::SPIRType& type = cmp.get_type(res.base_type_id);
		mDescriptorSets[set].emplace(bind,
			loadBinding(type, cmp));
	}

	mGLSLCode = cmp.compile();


	this->markUpdated(fc);
}



void Shader::BindingInfo::drawTableHeader()
{
	for (const char* c : cols) {
		ImGui::TableSetupColumn(c);
	}
	ImGui::TableHeadersRow();
}

void Shader::StructInfo::drawTableRow(uint32_t bind)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);

	const std::string bindName = "Bind " + std::to_string(bind);
	bool open = ImGui::TreeNodeEx(bindName.c_str(), ImGuiTreeNodeFlags_None);
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(this->name.c_str());
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Uniform Buffer");
	ImGui::TableNextColumn();
	ImGui::Text("%u", this->bytes);
	ImGui::TableNextColumn();
	ImGui::TextDisabled("--");

	if (open) {
		for (std::unique_ptr<BindingInfo>& p : childInfo) {
			p->drawTableRow(std::numeric_limits<uint32_t>::max());
		}

		ImGui::TreePop();
	}
}

void Shader::StructInfo::addChild(std::unique_ptr<BindingInfo>&& c)
{
	childInfo.push_back(std::move(c));
	bytes += childInfo.back()->bytes;
}

void Shader::BasicInfo::drawTableRow(uint32_t bind)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	if (bind != std::numeric_limits<uint32_t>::max()) {
		const std::string bindName = "Bind " + std::to_string(bind);
		ImGui::TreeNodeEx(bindName.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);
	}
	else {
		ImGui::TextDisabled("--");
	}
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(this->name.c_str());
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Base Type");
	ImGui::TableNextColumn();
	ImGui::Text("%u", this->bytes);
	ImGui::TableNextColumn();
	ImGui::Text("%u", this->offset);

}

void Shader::SampledImgInfo::drawTableRow(uint32_t bind)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	const std::string bindName = "Bind " + std::to_string(bind);
	ImGui::TreeNodeEx(bindName.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);
	
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(this->name.c_str());
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Sampled Image");
	ImGui::TableNextColumn();
	ImGui::Text("%u", this->bytes);
	ImGui::TableNextColumn();
	ImGui::Text("%u", this->offset);
}

} // namespace gr