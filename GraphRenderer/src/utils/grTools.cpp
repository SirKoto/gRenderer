#include "grTools.h"

#include <iostream>
#include <fstream>
#include <stb_image/stb_image.h>

namespace gr
{

void tools::loadBinaryFile(
	const char* fileName,
	std::vector<char>* outFileBytes)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	outFileBytes->resize(fileSize);

	file.seekg(0);
	file.read(outFileBytes->data(), fileSize);

	file.close();
}

void tools::loadImageRGBA(
	const char* fileName,
	uint8_t** outImg,
	uint32_t* outWidth, uint32_t* outHeight)
{
	int32_t width, height, chann;
	*outImg = stbi_load(
		fileName,
		&width, &height, &chann, STBI_rgb_alpha);

	*outWidth = static_cast<uint32_t>(width);
	*outHeight = static_cast<uint32_t>(height);
}

void tools::freeImage(uint8_t* img)
{
	stbi_image_free(img);
}

}; // namespace gr
