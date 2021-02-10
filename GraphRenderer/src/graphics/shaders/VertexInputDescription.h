#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace gr
{
namespace vkg
{


class VertexInputDescription
{
public:
	class Binding;
	Binding& addBinding(uint32_t bindId, uint32_t stride);
	
	std::vector<vk::VertexInputBindingDescription> getBindingDescription() const;

	std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() const;

	class Attribute
	{
	public:
		Attribute() = default;
		Attribute(uint32_t location, vk::Format format, uint32_t offset) :
		 mLocation(location), mFormat(format), mOffset(offset) {}

		void setLocation(uint32_t location) { mLocation = location; }
		void setFormat(vk::Format format) { mFormat = format; }
		void setOffset(uint32_t offset) { mOffset = offset; }

		const uint32_t& getLocation() const { return mLocation; }
		const vk::Format& getFormat() const { return mFormat; }
		const uint32_t& getOffset() const { return mOffset; }

	private:
		uint32_t mLocation;
		vk::Format mFormat;
		uint32_t mOffset;
	};

	// Input Rate Vertex
	class Binding
	{
	public:
		Binding() = default;
		Binding(uint32_t bindId, uint32_t stride) : mBindId(bindId), mStride(stride) {}

		void setBindId(uint32_t bindId) { mBindId = bindId; }
		void setStride(uint32_t stride) { mStride = stride; }

		const uint32_t& getBindId() const { return mBindId; }
		const uint32_t& getStride() const { return mStride; }
		const std::vector<Attribute>& getAttributes() const { return mAttributes; }

		Binding& addAttributeFloat(uint32_t location, uint32_t numFloats, uint32_t offset);
		Binding& addAttribute8UNORM(uint32_t location, uint32_t numUnsigned, uint32_t offset);

	private:
		uint32_t mBindId;
		uint32_t mStride;
		std::vector<Attribute> mAttributes;
	};

protected:
	std::vector<Binding> mBindings;

	const Binding& findBinding(uint32_t bindId) const;


};


}
}
