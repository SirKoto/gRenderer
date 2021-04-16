#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace gr {
namespace mth {

glm::vec3 to_euler(glm::quat q);

} // namespace mth
} // namespace gr