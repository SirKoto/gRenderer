#pragma once

#include <atomic>

namespace gr
{

namespace grjob
{

class Counter
{
public:

	Counter();

	Counter(uint32_t initialValue);

	uint32_t getValue() const;

	uint32_t decrement(uint32_t value);

	uint32_t increment(uint32_t value);

protected:

	std::atomic_int32_t mCounter;
};

} // namespace grjob

} // namespace gr