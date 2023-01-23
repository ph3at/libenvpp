#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <fmt/core.h>

#include <libenvpp/detail/environment.hpp>
#include <libenvpp/env.hpp>

namespace env {

TEST_CASE("Retrieving integer from testing environment", "[libenvpp_testing]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	const auto testing_env = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING_INT", "42"},
	};

	const auto _ = env::scoped_test_environment(testing_env);

	auto pre = env::prefix(prefix_name);
	const auto int_id = pre.register_variable<int>("INT");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE(parsed_and_validated_pre.ok());
	const auto int_val = parsed_and_validated_pre.get(int_id);
	REQUIRE(int_val.has_value());
	CHECK(*int_val == 42);
}

TEST_CASE("Retrieving integer with get from testing environment", "[libenvpp_testing]")
{
	const auto testing_env = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING_INT", "42"},
	};

	const auto _ = env::scoped_test_environment(testing_env);

	const auto int_value = get<int>("LIBENVPP_TESTING_INT");
	REQUIRE(int_value.has_value());
	CHECK(*int_value == 42);
}

TEST_CASE("Retrieving integer with get_or from testing environment", "[libenvpp_testing]")
{
	const auto testing_env = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING_INT", "42"},
	};

	const auto _ = env::scoped_test_environment(testing_env);

	const auto int_value = get_or<int>("LIBENVPP_TESTING_INT", 7);
	CHECK(int_value == 42);
}

} // namespace env
