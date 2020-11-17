#pragma once

#include <type_traits>
#include <tuple>
#include <memory>
#include <functional>

namespace gr
{

class Job
{
private:
	static const size_t SIZE = 10;
	typedef std::aligned_storage<sizeof(void*)* SIZE> Stack;
	Stack mBuffer;

	struct HolderBase
	{
		virtual ~HolderBase() = 0;
		virtual void operator()() = 0;
	};

	template<typename Functor, typename ...Args>
	struct Holder : HolderBase
	{
		Functor mF;
		std::tuple<Args...> mArgs;

		Holder(const Functor& f, Args... args)
		{
			new (std::addressof(mF)) Functor(f);
			new (std::addressof(mArgs)) std::tuple<Args...>(args...);
		}

		virtual void operator()() override {
			std::apply(mF, mArgs);
		}

		virtual ~Holder() = default;

	};

	template<typename Class, typename ...Args>
	struct HolderMember : HolderBase
	{
		using Fun_t = void(Class::*)(Args...);
		Fun_t mFun;
		Class* mClass;
		std::tuple<Args...> mArgs;

		HolderMember(Fun_t fun, Class* c, Args... args)
		{
			new(std::addressof(mFun)) Fun_t(fun);
			mClass = c;
			new (std::addressof(mArgs)) std::tuple<Args...>(args...);
		}

		virtual void operator()() override {
			std::apply(std::mem_fn(mFun), std::tuple_cat(std::make_tuple(mClass), mArgs));
		}

		virtual ~Holder() = default;

	};

public:

	inline void run() {
		reinterpret_cast<HolderBase&>(mBuffer)();
	}

	// only functions without return value supported

	template<typename Callable, typename ...Args>
	Job(Callable f, Args... args) {
		static_assert(sizeof(HolderBase<Callable, Args...>) <= sizeof(Stack), "Type does not fit the stack!!");

		new(std::addressof(mBuffer)) HolderBase(f, args...);
	}

	template<typename Callable, typename ...Args>
	Job(Callable* f, Args... args) {
		static_assert(sizeof(HolderBase<Callable::operator(), Args...>) <= sizeof(Stack), "Type does not fit the stack!!");

		new(std::addressof(mBuffer)) HolderBase(Callable::operator(), args...);
	}

	template<typename Class, typename ...Args>
	Job(void(Class::* callable)(Args...), Class* c, Args... args) {
		static_assert(sizeof(HolderMember<callable, c, Args...>) <= sizeof(Stack), "Type does not fit the stack!!");

		new(std::addressof(mBuffer)) HolderBase(callable, c, args...);
	}


};

} // namespace gr