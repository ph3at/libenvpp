#pragma once

#include <exception>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <fmt/core.h>

#include <libenvpp_util.hpp>

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

namespace helper {

template <typename T, typename = void>
struct has_callable_from_string_fn : std::false_type {};

template <typename T>
struct has_callable_from_string_fn<
    T, std::void_t<decltype(from_string(std::declval<std::string>(), std::declval<std::optional<T>&>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct has_rvalue_callable_from_string_fn : std::false_type {};

template <typename T>
struct has_rvalue_callable_from_string_fn<
    T, std::void_t<decltype(from_string(std::declval<std::string>(), std::declval<std::optional<T>&&>()))>>
    : std::true_type {};

} // namespace helper

template <typename T>
struct has_from_string_fn : std::conjunction<helper::has_callable_from_string_fn<T>,
                                             std::negation<helper::has_rvalue_callable_from_string_fn<T>>> {};

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

//////////////////////////////////////////////////////////////////////////

template <typename T>
std::string construct_from_string(const std::string_view str, std::optional<T>& value)
{
	try {
		value.reset();

		if constexpr (is_string_constructible_v<T>) {
			value.emplace(str);
		} else if constexpr (has_from_string_fn_v<T>) {
			if constexpr (std::is_same_v<decltype(from_string(str, value)), std::string>) {
				return from_string(str, value);
			} else {
				from_string(str, value);
			}
		} else if constexpr (is_stringstream_constructible_v<T>) {
			auto stream = std::istringstream(std::string(str));
			T parsed;
			stream >> parsed;
			if (!stream.fail()) {
				value = parsed;
			}
		} else {
			static_assert(
			    util::always_false_v<T>,
			    "Type is not constructible from string. Implement one of the supported construction mechanisms.");
		}
	} catch (const std::exception& e) {
		return fmt::format("Failed to construct object from string '{}' due to exception '{}'", str, e.what());
	} catch (...) {
		return fmt::format("Unknown exception occurred when constructing object from string '{}'", str);
	}

	return value.has_value() ? std::string{} : fmt::format("Failed to construct object from string '{}'", str);
}

} // namespace env::detail
