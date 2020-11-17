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

} // namespace cexprUtils

} // namespace gr