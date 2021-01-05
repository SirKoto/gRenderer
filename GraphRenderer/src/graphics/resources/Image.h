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

		vk::Image getVkImage() const { return mImage; }
		vk::ImageView getVkImageview() const { return mImageView; }

	protected:
		vk::Image mImage;
		vk::ImageView mImageView;
	};

}; // namespace vkg
}; // namespace gr
