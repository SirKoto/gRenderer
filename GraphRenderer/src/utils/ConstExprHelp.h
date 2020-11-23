#pragma once


namespace gr {

namespace cexprUtils
{

	constexpr size_t pow(size_t a, size_t e)
	{
		if (e == 0) {
			return 1;
		}
		if (e == 1)	{
			return a;
		}

		return a * pow(a, e - 1);
	}

	inline bool memEq(void* mem, unsigned char val, size_t size) {
		unsigned char* m = reinterpret_cast<unsigned char*>(mem);
		unsigned char* fin = m + size;
		for (; m != fin; m += 1) {
			if (*m != val) {
				return false;
			}
		}
		return true;
	}

} // namespace cexprUtils

} // namespace gr