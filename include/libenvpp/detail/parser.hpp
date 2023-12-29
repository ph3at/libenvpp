#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <fmt/core.h>

#include <libenvpp/detail/errors.hpp>
#include <libenvpp/detail/expected.hpp>
#include <libenvpp/detail/util.hpp>

namespace env {
namespace detail {

template <typename T>
struct is_string_constructible : std::conditional_t<                                 //
                                     std::is_constructible_v<T, std::string_view>    //
                                         || std::is_constructible_v<T, std::string>, //
                                     std::true_type,                                 //
                                     std::false_type> {
};

template <typename T>
inline constexpr auto is_string_constructible_v = is_string_constructible<T>::value;

//////////////////////////////////////////////////////////////////////////

template <typename T, typename = void>
struct has_stringstream_operator_right_shift : std::false_type {
};

template <typename T>
struct has_stringstream_operator_right_shift<
    T, std::void_t<decltype(std::declval<std::istringstream&>() >> std::declval<T&>())>> : std::true_type {
};

template <typename T>
inline constexpr auto has_stringstream_operator_right_shift_v = has_stringstream_operator_right_shift<T>::value;

template <typename T>
struct is_stringstream_constructible
    : std::conjunction<has_stringstream_operator_right_shift<T>, std::negation<std::is_same<T, char*>>,
                       std::negation<std::is_reference<T>>> {
};

template <typename T>
inline constexpr auto is_stringstream_constructible_v = is_stringstream_constructible<T>::value;

//////////////////////////////////////////////////////////////////////////

[[nodiscard]] static inline bool parse_bool(const std::string_view str)
{
	constexpr auto to_lower_char = [](const char c) -> char {
		if ('A' <= c && c <= 'Z') {
			return c + ('a' - 'A');
		} else {
			return c;
		}
	};

	constexpr auto equal_case_insensitive = [to_lower_char](const std::string_view a, const std::string_view b) {
		return std::equal(a.begin(), a.end(), b.begin(), b.end(),
		                  [to_lower_char](const char a, const char b) { return to_lower_char(a) == to_lower_char(b); });
	};

	if (equal_case_insensitive(str, "true")  //
	    || equal_case_insensitive(str, "on") //
	    || equal_case_insensitive(str, "yes")) {
		return true;
	} else if (equal_case_insensitive(str, "false")  //
	           || equal_case_insensitive(str, "off") //
	           || equal_case_insensitive(str, "no")) {
		return false;
	} else {
		throw parser_error{fmt::format("Failed to parse '{}' as boolean", str)};
	}
}

template <typename T>
[[nodiscard]] T construct_from_string(const std::string_view str)
{
	if constexpr (is_string_constructible_v<T>) {
		try {
			return T(std::string(str));
		} catch (const parser_error&) {
			throw;
		} catch (const std::exception& e) {
			throw parser_error{fmt::format("String constructor failed for input '{}' with '{}'", str, e.what())};
		} catch (...) {
			throw parser_error{fmt::format("String constructor failed for input '{}' with unknown error", str)};
		}
	} else if constexpr (is_stringstream_constructible_v<T>) {
		auto stream = std::istringstream(std::string(str));
		auto parsed = T();
		try {
			if constexpr (std::is_same_v<T, bool>) {
				stream >> parsed;
				if (stream.fail()) {
					stream = std::istringstream(std::string(str));
					std::string bool_str;
					stream >> bool_str;
					parsed = parse_bool(bool_str);
				}
			} else {
				stream >> parsed;
			}
			if (!stream.eof()) {
				stream >> std::ws;
			}
		} catch (const parser_error&) {
			throw;
		} catch (const std::exception& e) {
			throw parser_error{fmt::format("Stream operator>> failed for input '{}' with '{}'", str, e.what())};
		} catch (...) {
			throw parser_error{fmt::format("Stream operator>> failed for input '{}' with unknown error", str)};
		}
		if (stream.fail()) {
			throw parser_error{fmt::format("Stream operator>> failed for input '{}'", str)};
		}
		if (static_cast<std::size_t>(stream.tellg()) < str.size()) {
			throw parser_error{fmt::format("Input '{}' was only parsed partially with remaining data '{}'", str,
			                               stream.str().substr(stream.tellg()))};
		}
		if constexpr (!std::is_same_v<T, bool> && std::is_unsigned_v<T>) {
			auto signed_parsed = std::int64_t{};
			auto signed_stream = std::istringstream(std::string(str));
			signed_stream >> signed_parsed;
			if (!signed_stream.eof()) {
				signed_stream >> std::ws;
			}
			if (signed_stream.fail() || static_cast<std::size_t>(signed_stream.tellg()) < str.size()) {
				throw parser_error{
				    fmt::format("Failed to validate whether '{}' was correctly parsed as '{}'", str, parsed)};
			}
			if (signed_parsed < 0) {
				throw parser_error{fmt::format("Cannot parse negative number '{}' as unsigned type", str)};
			}
		}
		return parsed;
	} else {
		static_assert(util::always_false_v<T>,
		              "Type is not constructible from string. Implement one of the supported construction mechanisms.");
	}
}

} // namespace detail

template <typename T>
struct default_validator {
	void operator()(const T&) const noexcept {}
};

template <typename T>
struct default_parser {
	[[nodiscard]] T operator()(const std::string_view str) const { return detail::construct_from_string<T>(str); }
};

template <typename T>
struct default_parser_and_validator {
	[[nodiscard]] T operator()(const std::string_view str) const
	{
		const auto value = default_parser<T>{}(str);
		default_validator<T>{}(value);
		return value;
	}
};

namespace detail {

template <typename T, typename ParserAndValidator>
[[nodiscard]] expected<T, std::string> parse_or_error(const std::string_view env_var_name,
                                                      const std::string_view env_var_value,
                                                      ParserAndValidator&& parser_and_validator)
{
	using expected_t = expected<T, std::string>;
	using unexpected_t = typename expected_t::unexpected_type;

	auto error_msg = typename expected_t::error_type();

	try {
		return expected_t{parser_and_validator(env_var_value)};
	} catch (const parser_error& e) {
		error_msg = fmt::format("Parser error for environment variable '{}': {}", env_var_name, e.what());
	} catch (const validation_error& e) {
		error_msg = fmt::format("Validation error for environment variable '{}': {}", env_var_name, e.what());
	} catch (const range_error& e) {
		error_msg = fmt::format("Range error for environment variable '{}': {}", env_var_name, e.what());
	} catch (const option_error& e) {
		error_msg = fmt::format("Option error for environment variable '{}': {}", env_var_name, e.what());
	} catch (const std::exception& e) {
		error_msg =
		    fmt::format("Failed to parse or validate environment variable '{}' with: {}", env_var_name, e.what());
	} catch (...) {
		error_msg =
		    fmt::format("Failed to parse or validate environment variable '{}' with unknown error", env_var_name);
	}
	return expected_t{unexpected_t{error_msg}};
}

} // namespace detail

} // namespace env
