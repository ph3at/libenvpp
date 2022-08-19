#include <string>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include <libenvpp_util.hpp>

[[maybe_unused]] static auto g_lambda = []() {};

namespace env::detail::util {

TEST_CASE("Traits of lambdas without captures", "[libenvpp_util]")
{
	SECTION("Lambda storage class")
	{
		using g_lambda_traits = function_traits<decltype(g_lambda)>;
		static_assert(std::is_same_v<g_lambda_traits::result_type, void>);
		static_assert(g_lambda_traits::arity == 0);
		static_assert(std::is_same_v<g_lambda_traits::class_type, decltype(g_lambda)>);

		constexpr auto constexpr_lambda = []() {};
		using constexpr_lambda_traits = function_traits<decltype(constexpr_lambda)>;
		static_assert(std::is_same_v<constexpr_lambda_traits::result_type, void>);
		static_assert(constexpr_lambda_traits::arity == 0);
		static_assert(
		    std::is_same_v<std::add_const_t<constexpr_lambda_traits::class_type>, decltype(constexpr_lambda)>);

		const auto const_lambda = []() {};
		using const_lambda_traits = function_traits<decltype(const_lambda)>;
		static_assert(std::is_same_v<const_lambda_traits::result_type, void>);
		static_assert(const_lambda_traits::arity == 0);
		static_assert(std::is_same_v<std::add_const_t<const_lambda_traits::class_type>, decltype(const_lambda)>);

		volatile auto volatile_lambda = []() {};
		using volatile_lambda_traits = function_traits<decltype(volatile_lambda)>;
		static_assert(std::is_same_v<volatile_lambda_traits::result_type, void>);
		static_assert(volatile_lambda_traits::arity == 0);
		static_assert(
		    std::is_same_v<std::add_volatile_t<volatile_lambda_traits::class_type>, decltype(volatile_lambda)>);

		auto lambda = []() {};
		using lambda_traits = function_traits<decltype(lambda)>;
		static_assert(std::is_same_v<lambda_traits::result_type, void>);
		static_assert(lambda_traits::arity == 0);
		static_assert(std::is_same_v<lambda_traits::class_type, decltype(lambda)>);

		auto mutable_lambda = []() mutable {};
		using mutable_lambda_traits = function_traits<decltype(mutable_lambda)>;
		static_assert(std::is_same_v<mutable_lambda_traits::result_type, void>);
		static_assert(mutable_lambda_traits::arity == 0);
		static_assert(std::is_same_v<mutable_lambda_traits::class_type, decltype(mutable_lambda)>);

		static auto static_lambda = []() {};
		using static_lambda_traits = function_traits<decltype(static_lambda)>;
		static_assert(std::is_same_v<static_lambda_traits::result_type, void>);
		static_assert(static_lambda_traits::arity == 0);
		static_assert(std::is_same_v<static_lambda_traits::class_type, decltype(static_lambda)>);

		thread_local auto thread_local_lambda = []() {};
		using thread_local_lambda_traits = function_traits<decltype(thread_local_lambda)>;
		static_assert(std::is_same_v<thread_local_lambda_traits::result_type, void>);
		static_assert(thread_local_lambda_traits::arity == 0);
		static_assert(std::is_same_v<thread_local_lambda_traits::class_type, decltype(thread_local_lambda)>);

		[](auto& ref_lambda) {
			using ref_lambda_traits = function_traits<decltype(ref_lambda)>;
			static_assert(std::is_same_v<typename ref_lambda_traits::result_type, void>);
			static_assert(ref_lambda_traits::arity == 0);
			static_assert(std::is_same_v<std::add_lvalue_reference_t<typename ref_lambda_traits::class_type>,
			                             decltype(ref_lambda)>);
		}(lambda);

		[](const auto& const_ref_lambda) {
			using const_ref_lambda_traits = function_traits<decltype(const_ref_lambda)>;
			static_assert(std::is_same_v<typename const_ref_lambda_traits::result_type, void>);
			static_assert(const_ref_lambda_traits::arity == 0);
			static_assert(std::is_same_v<
			              std::add_lvalue_reference_t<std::add_const_t<typename const_ref_lambda_traits::class_type>>,
			              decltype(const_ref_lambda)>);
		}([]() {});

		[](auto&& rvalue_ref_lambda) {
			using rvalue_ref_lambda_traits = function_traits<decltype(rvalue_ref_lambda)>;
			static_assert(std::is_same_v<typename rvalue_ref_lambda_traits::result_type, void>);
			static_assert(rvalue_ref_lambda_traits::arity == 0);
			static_assert(std::is_same_v<std::add_rvalue_reference_t<typename rvalue_ref_lambda_traits::class_type>,
			                             decltype(rvalue_ref_lambda)>);
		}([]() {});
	}

	SECTION("Arguments and return type")
	{
		constexpr auto void_to_int_lambda = []() -> int { return {}; };
		using void_to_int_lambda_traits = function_traits<decltype(void_to_int_lambda)>;
		static_assert(std::is_same_v<void_to_int_lambda_traits::result_type, int>);
		static_assert(void_to_int_lambda_traits::arity == 0);

		constexpr auto int_to_void_lambda = [](int) {};
		using int_to_void_lambda_traits = function_traits<decltype(int_to_void_lambda)>;
		static_assert(std::is_same_v<int_to_void_lambda_traits::result_type, void>);
		static_assert(int_to_void_lambda_traits::arity == 1);
		static_assert(std::is_same_v<int_to_void_lambda_traits::arg0_type, int>);
		static_assert(std::is_same_v<int_to_void_lambda_traits::arg_type<0>, int>);

		constexpr auto int_to_int_lambda = [](int) -> int* { return {}; };
		using int_to_int_lambda_traits = function_traits<decltype(int_to_int_lambda)>;
		static_assert(std::is_same_v<int_to_int_lambda_traits::result_type, int*>);
		static_assert(int_to_int_lambda_traits::arity == 1);
		static_assert(std::is_same_v<int_to_int_lambda_traits::arg0_type, int>);
		static_assert(std::is_same_v<int_to_int_lambda_traits::arg_type<0>, int>);

		constexpr auto two_arg_lambda = [](int, float*) -> const char* { return {}; };
		using two_arg_lambda_traits = function_traits<decltype(two_arg_lambda)>;
		static_assert(std::is_same_v<two_arg_lambda_traits::result_type, const char*>);
		static_assert(two_arg_lambda_traits ::arity == 2);
		static_assert(std::is_same_v<two_arg_lambda_traits::arg0_type, int>);
		static_assert(std::is_same_v<two_arg_lambda_traits::arg1_type, float*>);
		static_assert(std::is_same_v<two_arg_lambda_traits::arg_type<0>, int>);
		static_assert(std::is_same_v<two_arg_lambda_traits::arg_type<1>, float*>);

		constexpr auto three_arg_lambda = [](const volatile int*, const float*, volatile char*) -> volatile float*
		{
			return {};
		};
		using three_arg_lambda_traits = function_traits<decltype(three_arg_lambda)>;
		static_assert(std::is_same_v<three_arg_lambda_traits::result_type, volatile float*>);
		static_assert(three_arg_lambda_traits::arity == 3);
		static_assert(std::is_same_v<three_arg_lambda_traits::arg0_type, const volatile int*>);
		static_assert(std::is_same_v<three_arg_lambda_traits::arg1_type, const float*>);
		static_assert(std::is_same_v<three_arg_lambda_traits::arg2_type, volatile char*>);
		static_assert(std::is_same_v<three_arg_lambda_traits::arg_type<0>, const volatile int*>);
		static_assert(std::is_same_v<three_arg_lambda_traits::arg_type<1>, const float*>);
		static_assert(std::is_same_v<three_arg_lambda_traits::arg_type<2>, volatile char*>);

		constexpr auto four_arg_lambda = [](int&, const float&, char&&, std::string) -> const volatile std::string*
		{
			return {};
		};
		using four_arg_lambda_traits = function_traits<decltype(four_arg_lambda)>;
		static_assert(std::is_same_v<four_arg_lambda_traits::result_type, const volatile std::string*>);
		static_assert(four_arg_lambda_traits::arity == 4);
		static_assert(std::is_same_v<four_arg_lambda_traits::arg0_type, int&>);
		static_assert(std::is_same_v<four_arg_lambda_traits::arg1_type, const float&>);
		static_assert(std::is_same_v<four_arg_lambda_traits::arg2_type, char&&>);
		static_assert(std::is_same_v<four_arg_lambda_traits::arg_type<0>, int&>);
		static_assert(std::is_same_v<four_arg_lambda_traits::arg_type<1>, const float&>);
		static_assert(std::is_same_v<four_arg_lambda_traits::arg_type<2>, char&&>);
		static_assert(std::is_same_v<four_arg_lambda_traits::arg_type<3>, std::string>);
	}
}

TEST_CASE("Traits of lambdas with captures", "[libenvpp_util]")
{
	auto value = 7;

	const auto lambda_with_value_capture = [value]() { static_cast<void>(value); };
	using lambda_with_value_capture_traits = function_traits<decltype(lambda_with_value_capture)>;
	static_assert(std::is_same_v<lambda_with_value_capture_traits::result_type, void>);
	static_assert(lambda_with_value_capture_traits::arity == 0);

	const auto lambda_with_implicit_value_capture = [=]() { static_cast<void>(value); };
	using lambda_with_implicit_value_capture_traits = function_traits<decltype(lambda_with_implicit_value_capture)>;
	static_assert(std::is_same_v<lambda_with_implicit_value_capture_traits::result_type, void>);
	static_assert(lambda_with_implicit_value_capture_traits::arity == 0);

	const auto lambda_with_mutable_value_capture = [value]() mutable { value = 4; };
	using lambda_with_mutable_value_capture_traits = function_traits<decltype(lambda_with_mutable_value_capture)>;
	static_assert(std::is_same_v<lambda_with_mutable_value_capture_traits::result_type, void>);
	static_assert(lambda_with_mutable_value_capture_traits::arity == 0);

	const auto lambda_with_ref_capture = [&value]() { value = 42; };
	using lambda_with_ref_capture_traits = function_traits<decltype(lambda_with_ref_capture)>;
	static_assert(std::is_same_v<lambda_with_ref_capture_traits::result_type, void>);
	static_assert(lambda_with_ref_capture_traits::arity == 0);

	const auto lambda_with_implicit_ref_capture = [&]() { static_cast<void>(value); };
	using lambda_with_implicit_ref_capture_traits = function_traits<decltype(lambda_with_implicit_ref_capture)>;
	static_assert(std::is_same_v<lambda_with_implicit_ref_capture_traits::result_type, void>);
	static_assert(lambda_with_implicit_ref_capture_traits::arity == 0);

	struct {
		int value;
		void f()
		{
			const auto lambda_with_this_ref_capture = [this]() { value = 1; };
			using lambda_with_this_ref_capture_traits = function_traits<decltype(lambda_with_this_ref_capture)>;
			static_assert(std::is_same_v<lambda_with_this_ref_capture_traits::result_type, void>);
			static_assert(lambda_with_this_ref_capture_traits::arity == 0);

			const auto lambda_with_this_value_capture = [*this]() { static_cast<void>(value); };
			using lambda_with_this_value_capture_traits = function_traits<decltype(lambda_with_this_value_capture)>;
			static_assert(std::is_same_v<lambda_with_this_value_capture_traits::result_type, void>);
			static_assert(lambda_with_this_value_capture_traits::arity == 0);
		}
		void c() const
		{
			const auto lambda_with_this_ref_capture = [this]() { static_cast<void>(value); };
			using lambda_with_this_ref_capture_traits = function_traits<decltype(lambda_with_this_ref_capture)>;
			static_assert(std::is_same_v<lambda_with_this_ref_capture_traits::result_type, void>);
			static_assert(lambda_with_this_ref_capture_traits::arity == 0);

			const auto lambda_with_this_value_capture = [*this]() { static_cast<void>(value); };
			using lambda_with_this_value_capture_traits = function_traits<decltype(lambda_with_this_value_capture)>;
			static_assert(std::is_same_v<lambda_with_this_value_capture_traits::result_type, void>);
			static_assert(lambda_with_this_value_capture_traits::arity == 0);
		}
	} foo;
	foo.f();
	foo.c();
}

static void void_to_void_fn() {}
static int void_to_int_fn()
{
	return {};
}
static void int_to_void_fn(int) {}
static int int_to_int_fn(int)
{
	return {};
}
[[maybe_unused]] static void noexcept_fn() noexcept {}

TEST_CASE("Traits of free standing functions", "[libenvpp_util]")
{
	SECTION("Function type")
	{
		using void_to_void_fn_traits = function_traits<decltype(void_to_void_fn)>;
		static_assert(std::is_same_v<void_to_void_fn_traits::result_type, void>);
		static_assert(void_to_void_fn_traits::arity == 0);

		using void_to_int_fn_traits = function_traits<decltype(void_to_int_fn)>;
		static_assert(std::is_same_v<void_to_int_fn_traits::result_type, int>);
		static_assert(void_to_int_fn_traits::arity == 0);

		using int_to_void_fn_traits = function_traits<decltype(int_to_void_fn)>;
		static_assert(std::is_same_v<int_to_void_fn_traits::result_type, void>);
		static_assert(int_to_void_fn_traits::arity == 1);
		static_assert(std::is_same_v<int_to_void_fn_traits::arg0_type, int>);
		static_assert(std::is_same_v<int_to_void_fn_traits::arg_type<0>, int>);

		using int_to_int_fn_traits = function_traits<decltype(int_to_int_fn)>;
		static_assert(std::is_same_v<int_to_int_fn_traits::result_type, int>);
		static_assert(int_to_int_fn_traits::arity == 1);
		static_assert(std::is_same_v<int_to_int_fn_traits::arg0_type, int>);
		static_assert(std::is_same_v<int_to_int_fn_traits::arg_type<0>, int>);

		using noexcept_fn_traits = function_traits<decltype(noexcept_fn)>;
		static_assert(std::is_same_v<noexcept_fn_traits::result_type, void>);
		static_assert(noexcept_fn_traits::arity == 0);
	}

	SECTION("Function pointer type")
	{
		auto void_to_void_fn_ptr = void_to_void_fn;
		const auto void_to_int_fn_ptr = void_to_int_fn;
		volatile auto int_to_void_fn_ptr = int_to_void_fn;
		const volatile auto int_to_int_fn_ptr = int_to_int_fn;

		using void_to_void_fn_ptr_traits = function_traits<decltype(void_to_void_fn_ptr)>;
		static_assert(std::is_same_v<void_to_void_fn_ptr_traits::result_type, void>);
		static_assert(void_to_void_fn_ptr_traits::arity == 0);

		using void_to_int_fn_ptr_traits = function_traits<decltype(void_to_int_fn_ptr)>;
		static_assert(std::is_same_v<void_to_int_fn_ptr_traits::result_type, int>);
		static_assert(void_to_int_fn_ptr_traits::arity == 0);

		using int_to_void_fn_ptr_traits = function_traits<decltype(int_to_void_fn_ptr)>;
		static_assert(std::is_same_v<int_to_void_fn_ptr_traits::result_type, void>);
		static_assert(int_to_void_fn_ptr_traits::arity == 1);
		static_assert(std::is_same_v<int_to_void_fn_ptr_traits::arg0_type, int>);
		static_assert(std::is_same_v<int_to_void_fn_ptr_traits::arg_type<0>, int>);

		using int_to_int_fn_ptr_traits = function_traits<decltype(int_to_int_fn_ptr)>;
		static_assert(std::is_same_v<int_to_int_fn_ptr_traits::result_type, int>);
		static_assert(int_to_int_fn_ptr_traits::arity == 1);
		static_assert(std::is_same_v<int_to_int_fn_ptr_traits::arg0_type, int>);
		static_assert(std::is_same_v<int_to_int_fn_ptr_traits::arg_type<0>, int>);
	}
}

TEST_CASE("Member function traits", "[libenvpp_util]")
{
	struct type {
		void void_to_void_mem_fn() {}
		int void_to_int_mem_fn() { return {}; }
		void int_to_void_mem_fn(int) {}
		int int_to_int_mem_fn(int) { return {}; }
	};

	SECTION("Member function type")
	{
		using void_to_void_mem_fn_traits = function_traits<decltype(&type::void_to_void_mem_fn)>;
		static_assert(std::is_same_v<void_to_void_mem_fn_traits::result_type, void>);
		static_assert(std::is_same_v<void_to_void_mem_fn_traits::class_type, type>);
		static_assert(void_to_void_mem_fn_traits::arity == 0);

		using void_to_int_mem_fn_traits = function_traits<decltype(&type::void_to_int_mem_fn)>;
		static_assert(std::is_same_v<void_to_int_mem_fn_traits::result_type, int>);
		static_assert(std::is_same_v<void_to_int_mem_fn_traits::class_type, type>);
		static_assert(void_to_int_mem_fn_traits::arity == 0);

		using int_to_void_mem_fn_traits = function_traits<decltype(&type::int_to_void_mem_fn)>;
		static_assert(std::is_same_v<int_to_void_mem_fn_traits::result_type, void>);
		static_assert(std::is_same_v<int_to_void_mem_fn_traits::class_type, type>);
		static_assert(int_to_void_mem_fn_traits::arity == 1);
		static_assert(std::is_same_v<int_to_void_mem_fn_traits::arg0_type, int>);
		static_assert(std::is_same_v<int_to_void_mem_fn_traits::arg_type<0>, int>);

		using int_to_int_mem_fn_traits = function_traits<decltype(&type::int_to_int_mem_fn)>;
		static_assert(std::is_same_v<int_to_int_mem_fn_traits::result_type, int>);
		static_assert(std::is_same_v<int_to_int_mem_fn_traits::class_type, type>);
		static_assert(int_to_int_mem_fn_traits::arity == 1);
		static_assert(std::is_same_v<int_to_int_mem_fn_traits::arg0_type, int>);
		static_assert(std::is_same_v<int_to_int_mem_fn_traits::arg_type<0>, int>);
	}

	struct specifiers {
		void mem_fn() {}
		void const_mem_fn() const {}
		void volatile_mem_fn() volatile {}
		void const_volatile_mem_fn() const volatile {}
		void ref_mem_fn() & {}
		void const_ref_mem_fn() const& {}
		void volatile_ref_mem_fn() volatile& {}
		void const_volatile_ref_mem_fn() const volatile& {}
		void rval_mem_fn() && {}
		void const_rval_mem_fn() const&& {}
		void volatile_rval_mem_fn() volatile&& {}
		void const_volatile_rval_mem_fn() const volatile&& {}
		void noexcept_mem_fn() noexcept {}
		void const_noexcept_mem_fn() const noexcept {}
		void volatile_noexcept_mem_fn() volatile noexcept {}
		void const_volatile_noexcept_mem_fn() const volatile noexcept {}
		void ref_noexcept_mem_fn() & noexcept {}
		void const_ref_noexcept_mem_fn() const& noexcept {}
		void volatile_ref_noexcept_mem_fn() volatile& noexcept {}
		void const_volatile_ref_noexcept_mem_fn() const volatile& noexcept {}
		void rval_noexcept_mem_fn() && noexcept {}
		void const_rval_noexcept_mem_fn() const&& noexcept {}
		void volatile_rval_noexcept_mem_fn() volatile&& noexcept {}
		void const_volatile_rval_noexcept_mem_fn() const volatile&& noexcept {}
	};

	SECTION("Member function specifiers")
	{
		constexpr auto all_mem_fn_specifier = std::tuple{&specifiers::mem_fn,
		                                                 &specifiers::const_mem_fn,
		                                                 &specifiers::volatile_mem_fn,
		                                                 &specifiers::const_volatile_mem_fn,
		                                                 &specifiers::ref_mem_fn,
		                                                 &specifiers::const_ref_mem_fn,
		                                                 &specifiers::volatile_ref_mem_fn,
		                                                 &specifiers::const_volatile_ref_mem_fn,
		                                                 &specifiers::rval_mem_fn,
		                                                 &specifiers::const_rval_mem_fn,
		                                                 &specifiers::volatile_rval_mem_fn,
		                                                 &specifiers::const_volatile_rval_mem_fn,
		                                                 &specifiers::noexcept_mem_fn,
		                                                 &specifiers::const_noexcept_mem_fn,
		                                                 &specifiers::volatile_noexcept_mem_fn,
		                                                 &specifiers::const_volatile_noexcept_mem_fn,
		                                                 &specifiers::ref_noexcept_mem_fn,
		                                                 &specifiers::const_ref_noexcept_mem_fn,
		                                                 &specifiers::volatile_ref_noexcept_mem_fn,
		                                                 &specifiers::const_volatile_ref_noexcept_mem_fn,
		                                                 &specifiers::rval_noexcept_mem_fn,
		                                                 &specifiers::const_rval_noexcept_mem_fn,
		                                                 &specifiers::volatile_rval_noexcept_mem_fn,
		                                                 &specifiers::const_volatile_rval_noexcept_mem_fn};

		for_constexpr(
		    [](auto&& fn) {
			    using fn_traits = function_traits<decltype(fn)>;
			    static_assert(std::is_same_v<typename fn_traits::result_type, void>);
			    static_assert(std::is_same_v<typename fn_traits::class_type, specifiers>);
			    static_assert(fn_traits::arity == 0);
		    },
		    all_mem_fn_specifier);
	}

	struct base {
		virtual void pure_virtual_mem_fn() = 0;
		virtual void virtual_mem_fn() {}
	};

	struct inherited : base {
		void pure_virtual_mem_fn() override {}
		void virtual_mem_fn() override final {}
	};

	SECTION("Inheritance member function specifiers")
	{
		using pure_virtual_mem_fn_traits = function_traits<decltype(&base::pure_virtual_mem_fn)>;
		static_assert(std::is_same_v<pure_virtual_mem_fn_traits::result_type, void>);
		static_assert(std::is_same_v<pure_virtual_mem_fn_traits::class_type, base>);
		static_assert(pure_virtual_mem_fn_traits::arity == 0);

		using virtual_mem_fn_traits = function_traits<decltype(&base::virtual_mem_fn)>;
		static_assert(std::is_same_v<virtual_mem_fn_traits::result_type, void>);
		static_assert(std::is_same_v<virtual_mem_fn_traits::class_type, base>);
		static_assert(virtual_mem_fn_traits::arity == 0);

		using inherited_pure_virtual_mem_fn_traits = function_traits<decltype(&inherited::pure_virtual_mem_fn)>;
		static_assert(std::is_same_v<inherited_pure_virtual_mem_fn_traits::result_type, void>);
		static_assert(std::is_same_v<inherited_pure_virtual_mem_fn_traits::class_type, inherited>);
		static_assert(inherited_pure_virtual_mem_fn_traits::arity == 0);

		using inherited_virtual_mem_fn_traits = function_traits<decltype(&inherited::virtual_mem_fn)>;
		static_assert(std::is_same_v<inherited_virtual_mem_fn_traits::result_type, void>);
		static_assert(std::is_same_v<inherited_virtual_mem_fn_traits::class_type, inherited>);
		static_assert(inherited_virtual_mem_fn_traits::arity == 0);
	}
}

} // namespace env::detail::util
