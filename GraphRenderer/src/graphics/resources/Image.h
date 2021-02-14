#pragma once

#include "Allocatable.h"

namespace gr
{
namespace vkg
{

	class Image : public Allocatable
	{
	public:
		void setImage(vk::Image image) { mImage = image; }
		void setImageView(vk::ImageView iv) { mImageView = iv; }

		const vk::Image& getVkImage() const { return mImage; }
		const vk::ImageView& getVkImageview() const { return mImageView; }

		operator bool() const { return mImage; }

	protected:
		vk::Image mImage;
		vk::ImageView mImageView;
	};

}; // namespace vkg
}; // namespace gr
