#include "BBox.h"

namespace gr {
namespace mth {

AABBox AABBox::operator*(const glm::mat4& transform) const
{
	AABBox o;

	for (uint32_t i = 0; i < 2; ++i) {
		for (uint32_t j = 0; j < 2; ++j) {
			for (uint32_t k = 0; k < 2; ++k) {
				glm::vec4 p(
					i == 0 ? mMin.x : mMax.x,
					j == 0 ? mMin.y : mMax.y,
					k == 0 ? mMin.z : mMax.z,
					1.0f
				);

				o.addPoint( glm::vec3(transform * p ));
			}
		}
	}

	return o;
}

} // namespace mth
} // namespace gr

