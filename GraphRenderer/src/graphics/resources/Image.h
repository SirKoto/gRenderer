#pragma once

#include "../memory/MemoryManager.h"
namespace gr
{
namespace vkg
{

	class Image
	{
	public:
		void setImage(vk::Image image) { mImage = image; }
		void setAllocation(VmaAllocation allocation) { mAllocation = allocation; }
		void setImageView(vk::ImageView iv) { mImageView = iv; }

		vk::Image getVkImage() const { return mImage; }
		VmaAllocation getAllocation() const { return mAllocation; }
		vk::ImageView getVkImageview() const { return mImageView; }

	protected:
		vk::Image mImage;
		vk::ImageView mImageView;
		VmaAllocation mAllocation;
	};

}; // namespace vkg
}; // namespace gr
