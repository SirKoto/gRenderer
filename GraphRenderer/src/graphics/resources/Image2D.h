#pragma once

#include "Image.h"

namespace gr
{
namespace vkg
{

	class Image2D : public Image
	{
	public:
		Image2D() = default;
		Image2D(uint32_t width, uint32_t height);

		void setExtent(vk::Extent2D extent) { mExtent = extent; }
		const vk::Extent2D& getExtent() const { return mExtent; }

	protected:
		vk::Extent2D mExtent;
	};

}; // namespace vkg
}; // namespace gr
