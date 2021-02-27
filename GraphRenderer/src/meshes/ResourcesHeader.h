#pragma once

#include <stdint.h>

#include "../utils/ConstExprHelp.h"


namespace gr
{

class Mesh;
class Texture;
class Sampler;
class DescriptorSetLayout;

// Set list of Types that the dictionary will handle
using ResourceTypesList =
typename ctools::TypelistBuilder<Mesh, Texture, Sampler,
	DescriptorSetLayout
>::typelist;

typedef uint64_t ResId;

}