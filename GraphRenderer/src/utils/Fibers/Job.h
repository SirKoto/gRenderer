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
	typedef std::aligned_storage<sizeof(void*)* SIZE>::type Stack;
	Stack mBuffer;


	struct HolderBase
	{
		virtual ~HolderBase() = default;
		virtual void operator()() = 0;
	};

	template<typename Callable, typename ...Args>
	struct Holder final : HolderBase
	{
		Callable mF;
		std::tuple<Args...> mArgs;

		Holder(Callable&& f, Args... args) : mF(std::forward<Callable>(f))
		{
			new (std::addressof(mArgs)) std::tuple<Args...>(args...);
		}

		virtual void operator()() override {
			std::apply(mF, mArgs);
		}

		virtual ~Holder() = default;

	};

	template<class TClass, typename ...Args>
	struct HolderMember final : HolderBase
	{
		void(TClass::* mFun)(Args...);

		TClass* mClass;
		std::tuple<Args...> mArgs;

		HolderMember(void(TClass::* fun)(Args...), TClass* c, Args... args) : mFun(fun)
		{
			mClass = c;
			new (std::addressof(mArgs)) std::tuple<Args...>(args...);
		}

		HolderMember(void(TClass::* fun)(Args...), TClass* c, Args&&... args) : mFun(fun)
		{
			mClass = c;
			new (std::addressof(mArgs)) std::tuple<Args...>(std::forward<Args>(args)...);
		}

		virtual void operator()() override {
			std::apply(std::mem_fn(mFun), std::tuple_cat(std::make_tuple(mClass), mArgs));
		}

		virtual ~HolderMember() = default;

	};


public:

	Job() {
		std::memset(&mBuffer, 0, sizeof(mBuffer));
	}

	~Job()
	{
		if (&reinterpret_cast<HolderBase&>(mBuffer) != nullptr) {
			reinterpret_cast<HolderBase&>(mBuffer).~HolderBase();
		}
	}

	void run() {
		assert((&reinterpret_cast<HolderBase&>(mBuffer) != nullptr));

		reinterpret_cast<HolderBase&>(mBuffer)();
	}

	// WARNING:
	// only functions without return value supported, and no arguments by reference!!!!
	// Functors (rvalues)
	
	template<typename Callable, typename ...Args> explicit
	Job(Callable&& f, Args... args) {
		static_assert(sizeof(Holder<Callable, Args...>) <= sizeof(Stack), "Type does not fit the stack!!");

		new(std::addressof(mBuffer)) Holder(std::forward<Callable>(f), args...);
	}

	// Function pointers
	template<typename ...Args> explicit
	Job(void(* f)(Args...), Args... args) {
		static_assert(sizeof(Holder<void(*)(Args...), Args...>) <= sizeof(Stack), "Type does not fit the stack!!");

		new(std::addressof(mBuffer)) Holder(f, args...);
	}

	// Member functions
	template<class TClass, typename ...Args> explicit
	Job(void(TClass::* callable)(Args...), TClass* c, Args&&... args) {
		static_assert(sizeof(HolderMember<TClass, Args...>) <= sizeof(Stack), "Type does not fit the stack!!");

		new(std::addressof(mBuffer)) HolderMember(callable, c, std::forward<Args>(args)...);
	}

};

} // namespace gr