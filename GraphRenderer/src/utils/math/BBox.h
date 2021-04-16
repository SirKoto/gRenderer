#pragma once

#include <glm/glm.hpp>
#include <numeric>
#include <string>

namespace gr {
namespace mth {

class AABBox {
public:

	AABBox() : mMin(std::numeric_limits<float>::infinity()),
		mMax(-std::numeric_limits<float>::infinity()) {}

	inline const glm::vec3& getMin() const { return mMin; }
	inline const glm::vec3& getMax() const { return mMax; }
	inline glm::vec3 getSize() const { return mMax - mMin; }

	inline void addPoint(const glm::vec3& p) {
		mMin = glm::min(mMin, p);
		mMax = glm::max(mMax, p);
	}

	inline void reset() {
		mMin = glm::vec3(std::numeric_limits<float>::infinity());
		mMax = glm::vec3(-std::numeric_limits<float>::infinity());
	}


private:
	glm::vec3 mMin;
	glm::vec3 mMax;

};
} // namespace mth

} // namespace gr
