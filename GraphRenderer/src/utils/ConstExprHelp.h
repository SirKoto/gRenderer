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

	struct VoidType {};
	

	// TypeList implementation
	// call TypelistBuilder<classes...>::typelist
	// constexpr function length<TypeList>() to get length of typelist
	// struct TypeAt<TypeList, idx>::type to get type at index
	// constexpr function indexOf<TypeList, Class> to get idx of Class in typelist

	template<typename H, typename T>
	struct TypeList {
		typedef H head;
		typedef T tail;
	};

	template <typename H, typename ...Ls>
	struct TypelistBuilder_ {
		using newType = typename TypelistBuilder_<Ls...>::type;
		using type = typename TypeList<H, newType>;
	};

	template <>
	struct TypelistBuilder_<VoidType> {
		using newType = typename VoidType;
		using type = typename VoidType;
	};

	template <typename ...Ls>
	struct TypelistBuilder {
		using typelist = typename TypelistBuilder_<Ls..., VoidType>::type;
	};

	// length of typelist
	template<typename TList>
	struct Length;

	template<>
	struct Length<VoidType> {
		enum { value = 0 };
	};

	template<typename H, typename T>
	struct Length< TypeList<H, T>> {
		enum { value = 1 + Length<T>::value };
	};

	template<typename TList>
	constexpr size_t length() { return Length<TList>::value; }

	// indexed access
	template<typename TList, size_t idx>
	struct TypeAt;

	template<typename H, typename T>
	struct TypeAt<TypeList<H, T>, 0> {
		using type = typename H;
	};
	template<typename H, typename T, size_t idx>
	struct TypeAt<TypeList<H, T>, idx> {
		using type = typename TypeAt<T, idx - 1>::type;
	};

	// get index of type
	template<typename TList, typename Class>
	struct IndexOf;

	template<typename Class>
	struct IndexOf<VoidType, Class> {
		enum { value = -1 };
	};

	template<typename T, typename Class>
	struct IndexOf<TypeList<Class, T>, Class> {
		enum { value = 0 };
	};

	template<typename H, typename T, typename Class>
	struct IndexOf<TypeList<H, T>, Class> {
	private:
		enum { tmp = IndexOf<T, Class>::value };
	public:
		enum { value = (tmp == -1) ? -1 : 1 + tmp };
	};

	template<typename TList, typename Class>
	constexpr size_t indexOf() { return IndexOf<TList, Class>::value; }


} // namespace cexprUtils

} // namespace gr