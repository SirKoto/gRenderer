#pragma once
#include <string>
#include <vector>

namespace gr
{
namespace tools
{

void loadBinaryFile(
	const std::string& fileName,
	std::vector<char>* outFileBytes
);

}; // namespace tools 
}; // namespace gr

