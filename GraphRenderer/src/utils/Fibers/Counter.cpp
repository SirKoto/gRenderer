#include "Counter.h"


namespace gr
{
namespace grjob
{
Counter::Counter()
{
	mCounter.store(0, std::memory_order_relaxed);
}

Counter::Counter(uint32_t initialValue)
{
	mCounter.store(initialValue, std::memory_order_relaxed);
}

uint32_t Counter::getValue() const
{
	return mCounter.load(std::memory_order_relaxed);
}

uint32_t Counter::decrement(uint32_t value)
{
	return mCounter.fetch_sub(value, std::memory_order_relaxed);
}

uint32_t Counter::increment(uint32_t value)
{
	return mCounter.fetch_add(value, std::memory_order_relaxed);
}

} // namespace grjob
} // namespace gr