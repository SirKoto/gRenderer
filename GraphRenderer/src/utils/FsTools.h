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

}; // namespace tools 
}; // namespace gr

