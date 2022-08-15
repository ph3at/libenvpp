#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace env::detail {

template <typename T>
struct is_string_constructible : std::conditional_t<                                 //
                                     std::is_constructible_v<T, std::string_view>    //
                                         || std::is_constructible_v<T, std::string>, //
                                     std::true_type,                                 //
                                     std::false_type> {};

template <typename T>
inline constexpr auto is_string_constructible_v = is_string_constructible<T>::value;

//////////////////////////////////////////////////////////////////////////

template <typename T, typename = void>
struct has_from_string_mem_fn : std::false_type {};

template <typename T>
struct has_from_string_mem_fn<T, std::void_t<decltype(T::from_string(std::declval<std::string>()))>> : std::true_type {
};

template <typename T>
inline constexpr auto has_from_string_mem_fn_v = has_from_string_mem_fn<T>::value;

//////////////////////////////////////////////////////////////////////////

template <typename T, typename = void>
struct has_from_string_fn : std::false_type {};

template <typename T>
struct has_from_string_fn<
    T, std::void_t<decltype(from_string(std::declval<std::string>(), std::declval<std::optional<T>&>()))>>
    : std::true_type {};

template <typename T>
inline constexpr auto has_from_string_fn_v = has_from_string_fn<T>::value;

//////////////////////////////////////////////////////////////////////////

template <typename T, typename = void>
struct is_stringstream_constructible : std::false_type {};

template <typename T>
struct is_stringstream_constructible<T,
                                     std::void_t<decltype(std::declval<std::istringstream&>() >> std::declval<T&>())>>
    : std::true_type {};

template <typename T>
inline constexpr auto is_stringstream_constructible_v = is_stringstream_constructible<T>::value;

} // namespace env::detail
