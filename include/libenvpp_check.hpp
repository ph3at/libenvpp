#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include <fmt/core.h>

#ifndef LIBENVPP_STRINGIFY
#define LIBENVPP_STRINGIFY_HELPER(expression) #expression
#define LIBENVPP_STRINGIFY(expression) LIBENVPP_STRINGIFY_HELPER(expression)
#endif

namespace env::detail {
class check_failed : public std::runtime_error {
  public:
	check_failed(const std::string_view error) : std::runtime_error(std::string(error)) {}
};
} // namespace env::detail

#if LIBENVPP_CHECKS_ENABLED && !LIBENVPP_TESTS_ENABLED
#define LIBENVPP_CHECK(condition) \
	do { \
		if (!(condition)) { \
			::fmt::print(stderr, "{}:{}: {}(): 'LIBENVPP_CHECK(" LIBENVPP_STRINGIFY(condition) ")' failed.", __FILE__, \
			             __LINE__, __func__); \
			::std::abort(); \
		} \
	} while (false)
#elif LIBENVPP_CHECKS_ENABLED && LIBENVPP_TESTS_ENABLED
#define LIBENVPP_CHECK(condition) \
	do { \
		if (!(condition)) { \
			throw ::env::detail::check_failed{ \
			    ::fmt::format("{}:{}: {}(): 'LIBENVPP_CHECK(" LIBENVPP_STRINGIFY(condition) ")' failed.", __FILE__, \
			                  __LINE__, __func__)}; \
		} \
	} while (false)
#else
#define LIBENVPP_CHECK(condition)
#endif
