#pragma once
#include <string>
#include <vector>

namespace gr
{
namespace tools
{

void loadBinaryFile(
	const char* fileName,
	std::vector<char>* outFileBytes
);

void loadImageRGBA(
	const char* fileName,
	uint8_t** outImg,
	uint32_t* outWidth,
	uint32_t* outHeight
);

void freeImage(uint8_t* img);

}; // namespace tools 
}; // namespace gr

