#pragma once

#include <cereal/types/polymorphic.hpp>

// only to use with binary or json encoding
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>

#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/unordered_set.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/memory.hpp>

#include <cereal/access.hpp>

// To use the serialization, look at https://uscilab.github.io/cereal/polymorphism.html

#define GR_SERIALIZE_TYPE(T) CEREAL_REGISTER_TYPE(T)
#define GR_SERIALIZE_TYPE_WITH_NAME(T, name) CEREAL_REGISTER_TYPE_WITH_NAME(T, name)
#define GR_SERIALIZE_POLYMORPHIC_RELATION(Base, Derived) CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, Derived)
// Create Name Value Pair of Member, with a tag that removes the first char of the name
#define GR_SERIALIZE_NVP_MEMBER(Member) cereal::make_nvp((#Member) + 1, Member)

#define GR_SERIALIZE_PRIVATE_MEMBERS friend class cereal::access;


// GLM serialization

#include <glm/glm.hpp>

namespace cereal
{
// vec
template<typename Archive, typename T, glm::qualifier Q>
void serialize(Archive& archive,
    glm::vec<3, T, Q>& m)
{
    archive(
        cereal::make_nvp("x", m.x),
        cereal::make_nvp("y", m.y),
        cereal::make_nvp("z", m.z));
}
template<typename Archive, typename T, glm::qualifier Q>
void serialize(Archive& archive,
    glm::vec<2, T, Q>& m)
{
    archive(
        cereal::make_nvp("x", m.x),
        cereal::make_nvp("y", m.y));
}

template<typename Archive, typename T, glm::qualifier Q>
void serialize(Archive& archive,
    glm::qua<T, Q>& m)
{
    archive(
        cereal::make_nvp("w", m.w),
        cereal::make_nvp("x", m.x),
        cereal::make_nvp("y", m.y),
        cereal::make_nvp("z", m.z));
}

} // namespace cereal