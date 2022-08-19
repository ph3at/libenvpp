#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace env::detail::util {

//////////////////////////////////////////////////////////////////////////
// Always false helper for static assertions.

template <typename...>
struct always_false : std::false_type {};
template <typename... Ts>
inline constexpr auto always_false_v = always_false<Ts...>::value;

//////////////////////////////////////////////////////////////////////////
// Compile time loop.

template <typename Fn, std::size_t... Idxs>
constexpr decltype(auto) for_constexpr(Fn&& func, std::index_sequence<Idxs...>)
{
	// Each iteration returns nothing.
	if constexpr ((std::is_void_v<std::invoke_result_t<Fn, std::integral_constant<std::size_t, Idxs>>> && ...)) {
		(func(std::integral_constant<std::size_t, Idxs>{}), ...);
	}
	// Each iteration returns a bool, allowing for short-circuit evaluation, and the result is returned from the loop.
	else if constexpr ((std::is_same_v<std::invoke_result_t<Fn, std::integral_constant<std::size_t, Idxs>>,
	                                   bool> && ...)) {
		if ((func(std::integral_constant<std::size_t, Idxs>{}) && ...)) {
			return true;
		}
		return false;
	}
	// Each iteration returns arbitrary non-void type which will be returned as a tuple from the loop.
	else if constexpr ((!std::is_void_v<std::invoke_result_t<Fn, std::integral_constant<std::size_t, Idxs>>> && ...)) {
		return std::tuple{func(std::integral_constant<std::size_t, Idxs>{})...};
	} else {
		static_assert(always_false_v<Fn>,
		              "All control paths must either return void, bool, or arbitrary non-void types");
	}
}

// Overload to allow iterating over tuple elements.
template <typename Fn, typename Tuple>
constexpr decltype(auto) for_constexpr(Fn&& func, Tuple&& tuple)
{
	return for_constexpr([&](const auto idx) { return func(std::get<idx.value>(tuple)); },
	                     std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

//////////////////////////////////////////////////////////////////////////
// Function traits for free standing functions, function pointers,
// member functions, and lambdas.

template <typename T, typename = void>
struct get_call_operator {
	using type = T;
};

template <typename T>
struct get_call_operator<T, std::void_t<decltype(&std::remove_reference_t<T>::operator())>> {
	using type = decltype(&std::remove_reference_t<T>::operator());
};

template <typename T>
using get_call_operator_t = typename get_call_operator<T>::type;

// clang-format off
template <typename T>                            struct remove_mem_fn_specifier                                          { using type = T; };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...)>                           { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const>                     { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) volatile>                  { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const volatile>            { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) &>                         { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const&>                    { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) volatile&>                 { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const volatile&>           { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) &&>                        { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const&&>                   { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) volatile&&>                { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const volatile&&>          { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) noexcept>                  { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const noexcept>            { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) volatile noexcept>         { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const volatile noexcept>   { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) & noexcept>                { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const& noexcept>           { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) volatile& noexcept>        { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const volatile& noexcept>  { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) && noexcept>               { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const&& noexcept>          { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) volatile&& noexcept>       { using type = R(C::*)(A...); };
template <typename C, typename R, typename... A> struct remove_mem_fn_specifier<R(C::*)(A...) const volatile&& noexcept> { using type = R(C::*)(A...); };
template <typename T> using remove_mem_fn_specifier_t = typename remove_mem_fn_specifier<T>::type;

template <typename T>                struct remove_fn_specifier                   { using type = T; };
template <typename R, typename... A> struct remove_fn_specifier<R(A...) noexcept> { using type = R(A...); };
template <typename T> using remove_fn_specifier_t = typename remove_fn_specifier<T>::type;
// clang-format on

template <typename Fn>
struct function_traits
    : function_traits<remove_mem_fn_specifier_t<remove_fn_specifier_t<
          std::remove_pointer_t<std::remove_cv_t<std::remove_reference_t<get_call_operator_t<Fn>>>>>>> {};

template <typename C, typename R, typename... A>
struct function_traits<R (C::*)(A...)> : function_traits<R(A...)> {
	using class_type = C;
};

template <typename R>
struct function_traits<R()> {
	using result_type = R;
	static constexpr std::size_t arity = 0;
};

template <typename R, typename A0>
struct function_traits<R(A0)> {
	using result_type = R;
	using arg0_type = A0;
	template <std::size_t Index>
	using arg_type = typename std::tuple_element_t<Index, std::tuple<A0>>;
	static constexpr std::size_t arity = 1;
};

template <typename R, typename A0, typename A1>
struct function_traits<R(A0, A1)> {
	using result_type = R;
	using arg0_type = A0;
	using arg1_type = A1;
	template <std::size_t Index>
	using arg_type = typename std::tuple_element_t<Index, std::tuple<A0, A1>>;
	static constexpr std::size_t arity = 2;
};

template <typename R, typename A0, typename A1, typename A2>
struct function_traits<R(A0, A1, A2)> {
	using result_type = R;
	using arg0_type = A0;
	using arg1_type = A1;
	using arg2_type = A2;
	template <std::size_t Index>
	using arg_type = typename std::tuple_element_t<Index, std::tuple<A0, A1, A2>>;
	static constexpr std::size_t arity = 3;
};

template <typename R, typename A0, typename A1, typename A2, typename... A>
struct function_traits<R(A0, A1, A2, A...)> {
	using result_type = R;
	using arg0_type = A0;
	using arg1_type = A1;
	using arg2_type = A2;
	template <std::size_t Index>
	using arg_type = typename std::tuple_element_t<Index, std::tuple<A0, A1, A2, A...>>;
	static constexpr std::size_t arity = 3 + sizeof...(A);
};

} // namespace env::detail::util
