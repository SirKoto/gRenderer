#include "VertexInputDescription.h"

#include <algorithm>

gr::vkg::VertexInputDescription::Binding& gr::vkg::VertexInputDescription::Binding::addAttributeFloat(uint32_t location, uint32_t numFloats, uint32_t offset)
{

	vk::Format format;
	switch (numFloats)
	{
	case 1:
		format = vk::Format::eR32Sfloat;
		break;
	case 2:
		format = vk::Format::eR32G32Sfloat;
		break;
	case 3:
		format = vk::Format::eR32G32B32Sfloat;
		break;
	case 4:
		format = vk::Format::eR32G32B32A32Sfloat;
		break;
	default:
		throw std::runtime_error("Binding attribute with non supported format");
		break;
	}

	mAttributes.emplace_back(location, format, offset);
	return *this;
}

gr::vkg::VertexInputDescription::Binding& gr::vkg::VertexInputDescription::Binding::addAttribute8UNORM(uint32_t location, uint32_t numUnsigned, uint32_t offset)
{
	vk::Format format;
	switch (numUnsigned)
	{
	case 1:
		format = vk::Format::eR8Unorm;
		break;
	case 2:
		format = vk::Format::eR8G8Unorm;
		break;
	case 3:
		format = vk::Format::eR8G8B8Unorm;
		break;
	case 4:
		format = vk::Format::eR8G8B8A8Unorm;
		break;
	default:
		throw std::runtime_error("Binding attribute with non supported format");
		break;
	}

	mAttributes.emplace_back(location, format, offset);
	return *this;
}

gr::vkg::VertexInputDescription::Binding& gr::vkg::VertexInputDescription::addBinding(uint32_t bindId, uint32_t stride)
{

	mBindings.emplace_back(bindId, stride);

	return mBindings.back();
}

std::vector<vk::VertexInputBindingDescription> gr::vkg::VertexInputDescription::getBindingDescription() const
{
	const uint32_t size = static_cast<uint32_t>(mBindings.size());
	std::vector<vk::VertexInputBindingDescription> descs(size);

	for (uint32_t i = 0; i < size; ++i) {
		vk::VertexInputBindingDescription& desc = descs[i];
		const Binding& binding = mBindings[i];
		desc.binding = binding.getBindId();
		desc.stride = binding.getStride();
		desc.inputRate = vk::VertexInputRate::eVertex;
	}

	return descs;
}

std::vector<vk::VertexInputAttributeDescription> gr::vkg::VertexInputDescription::getAttributeDescriptions() const
{
	const uint32_t numBind = static_cast<uint32_t>(mBindings.size());
	uint32_t numAttribs = 0;

	for (const Binding& bind : mBindings) {
		numAttribs += static_cast<uint32_t>(bind.getAttributes().size());
	}

	std::vector<vk::VertexInputAttributeDescription> attribDesc(numAttribs);

	numAttribs = 0;
	for (const Binding& bind : mBindings) {
		for (const Attribute& attrib : bind.getAttributes()) {

			

			attribDesc[numAttribs] = vk::VertexInputAttributeDescription(
				attrib.getLocation(), bind.getBindId(), attrib.getFormat(), attrib.getOffset()
			);

			numAttribs += 1;
		}
	}

	return attribDesc;
}

bool gr::vkg::VertexInputDescription::existsBinding(uint32_t bindId) const
{
	const uint32_t size = static_cast<uint32_t>(mBindings.size());
	for (uint32_t i = 0; i < size; ++i) {
		if (mBindings[i].getBindId() == bindId) {
			return true;
		}
	}

	return false;
}

const gr::vkg::VertexInputDescription::Binding& gr::vkg::VertexInputDescription::findBinding(uint32_t bindId) const
{
	const uint32_t size = static_cast<uint32_t>(mBindings.size());
	for (uint32_t i = 0; i < size; ++i) {
		if (mBindings[i].getBindId() == bindId) {
			return mBindings[i];
		}
	}

	throw std::runtime_error("Error: VertexInput Description: Trying to find unnexisting binding");
}
