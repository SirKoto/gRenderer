#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <mutex>
namespace gr {

class Logger
{
public:
	Logger();
	void clear();

	void addLog(std::string log);

	void drawImGui();

private:
	std::vector<size_t> mOffsets;
	std::vector<char> mBuffer;

	mutable std::mutex mMutex;
};

}

