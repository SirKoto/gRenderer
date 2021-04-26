#pragma once

#include <stdint.h>
#include <limits>
#include <functional>
#include <string>
#include "../utils/serialization.h"

#include "../utils/ConstExprHelp.h"


namespace gr
{

class Mesh;
class Texture;
class Sampler;
class DescriptorSetLayout;
class Shader;
class Material;
class Pipeline;
class Scene;
class GameObject;

// Set list of Types that the dictionary will handle
using ResourceTypesList =
typename ctools::TypelistBuilder<Mesh, Texture, Sampler,
	DescriptorSetLayout, Shader, Pipeline, Material, Scene, GameObject
>::typelist;

struct ResId {

	uint64_t value = std::numeric_limits<uint64_t>::max();

	operator bool() const {
		return this->value != std::numeric_limits<uint64_t>::max();
	}

	bool operator < (const ResId& o) const {
		return this->value < o.value;
	}

	bool operator == (const ResId& o) const {
		return this->value == o.value;
	}

	bool operator != (const ResId& o) const {
		return !(*this == o);
	}

	ResId() = default;
	explicit ResId(uint64_t v) : value(v) {}

	void reset() {
		this->value = std::numeric_limits<uint64_t>::max();
	}

	// serialization
	template <class Archive>
	uint64_t save_minimal(
		Archive const&) const
	{
		return this->value;
	}

	template <class Archive>
	void load_minimal(Archive const&,
		uint64_t const& value)
	{
		this->value = value;
	}
};

}

namespace std
{

template <>
struct hash<gr::ResId> {
	size_t operator ()(const gr::ResId& id) const {
		return hash<uint64_t>()(id.value);
	}
};

inline std::string to_string(const gr::ResId& id) {
	return std::to_string(id.value);
}

}