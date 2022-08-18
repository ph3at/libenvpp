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
// Function traits for free standing functions, function pointers,
// member functions, and lambdas.

template <typename Fn>
struct function_traits : function_traits<decltype(&std::remove_reference_t<Fn>::operator())> {};

// clang-format off
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...)>                           : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const>                     : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) volatile>                  : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) noexcept>                  : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const volatile>            : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const noexcept>            : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const volatile noexcept>   : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) &>                         : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) & noexcept>                : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const&>                    : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const& noexcept>           : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) volatile&>                 : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) volatile& noexcept>        : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const volatile&>           : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const volatile& noexcept>  : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) &&>                        : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) && noexcept>               : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const&&>                   : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const&& noexcept>          : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) volatile&&>                : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) volatile&& noexcept>       : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const volatile&&>          : function_traits<R(A...)> { using class_type = C; };
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const volatile&& noexcept> : function_traits<R(A...)> { using class_type = C; };

template <typename R, typename... A> struct function_traits<R(*)(A...)>               : function_traits<R(A...)> {};
template <typename R, typename... A> struct function_traits<R(*const)(A...)>          : function_traits<R(A...)> {};
template <typename R, typename... A> struct function_traits<R(*volatile)(A...)>       : function_traits<R(A...)> {};
template <typename R, typename... A> struct function_traits<R(*const volatile)(A...)> : function_traits<R(A...)> {};
// clang-format on

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
