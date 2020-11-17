#include "FsTools.h"

#include <iostream>
#include <fstream>

namespace gr
{

void tools::loadBinaryFile(
	const std::string& fileName,
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

}; // namespace gr
