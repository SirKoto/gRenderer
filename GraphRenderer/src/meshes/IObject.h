#pragma once

#include <string>

namespace gr
{
class IObject
{
public:

	const std::string& getName() const { return mName; }
	void setName(const std::string& name) { mName = name; }

protected:

	std::string mName;
};

} // namespace gr