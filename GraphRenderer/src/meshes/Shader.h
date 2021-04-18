#pragma once
#include "IObject.h"

#include <string>
#include <vulkan/vulkan.hpp>
#include <map>
#include <glm/glm.hpp>

namespace gr
{


class Shader :
    public IObject
{
public:

    Shader() = default;
    Shader(FrameContext* fc) : IObject(fc) {}


    virtual void scheduleDestroy(FrameContext* fc) override;
    virtual void renderImGui(FrameContext* fc, GuiFeedback* feedback = nullptr) override;

    static constexpr const char* s_getClassName() { return "Shader"; }

    void load(FrameContext* fc, const char* filePath);

    enum class Type {
        eFloat
    };

    struct LocationInfo {
        Type type;
        uint32_t vecSize;
        std::string name;
    };
    struct InLocationInfo : LocationInfo {
        VertexInputFlags inputFlags;
    };

    struct BindingInfo {
        static constexpr std::array<const char*, 5> cols =
        { "Pos", "Name", "Type", "Bytes", "Offset" };
        static void drawTableHeader();
        virtual void drawTableRow(uint32_t bind) = 0;
        virtual void addChild(std::unique_ptr<BindingInfo>&& c) {}
        virtual void setOffset(uint32_t offset) { this->offset = offset; }
        virtual ~BindingInfo() = default;

        std::string name = "";
        uint32_t bytes = 0, offset = 0;

    };

    struct StructInfo : public BindingInfo {

        virtual void drawTableRow(uint32_t bind) override final;
        virtual void addChild(std::unique_ptr<BindingInfo>&& c) override final;

        std::vector<std::unique_ptr<BindingInfo>> childInfo;
    };

    struct BasicInfo : public BindingInfo {
        virtual void drawTableRow(uint32_t bind) override final;
    };

    struct SampledImgInfo : public BindingInfo {
        virtual void drawTableRow(uint32_t bind) override final;
    };

private:

    vk::ShaderModule mShaderModule;

    vk::ShaderStageFlagBits mStage = vk::ShaderStageFlagBits::eVertex;

    std::string mPath;

    std::string mGLSLCode;


    std::map<uint32_t, InLocationInfo> mInLocations;
    std::map<uint32_t, LocationInfo> mOutLocations;

    typedef std::map<uint32_t, std::unique_ptr<BindingInfo>> DescriptorSet;
    std::map<uint32_t, DescriptorSet> mDescriptorSets;

    // Serialization functions
    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(cereal::base_class<IObject>(this));
        //TODO
    }

    GR_SERIALIZE_PRIVATE_MEMBERS

}; // class Shader


} // namespace gr


GR_SERIALIZE_TYPE(gr::Shader)
GR_SERIALIZE_POLYMORPHIC_RELATION(gr::IObject, gr::Shader)