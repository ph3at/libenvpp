#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <fmt/core.h>

#include <libenvpp/detail/environment.hpp>
#include <libenvpp/env.hpp>

namespace env {

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::StartsWith;

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

TEST_CASE("Testing environment is cleared after scope", "[libenvpp_testing]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	{
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

	auto pre = env::prefix(prefix_name);
	[[maybe_unused]] const auto int_id = pre.register_required_variable<int>("INT");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE_FALSE(parsed_and_validated_pre.ok());
	CHECK_THAT(parsed_and_validated_pre.error_message(), StartsWith("Error") && ContainsSubstring(prefix_name)
	                                                         && ContainsSubstring("'LIBENVPP_TESTING_INT'")
	                                                         && ContainsSubstring("not set"));
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

TEST_CASE("Multiple scoped test environments", "[libenvpp_testing]")
{
	const auto testing_env1 = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING1_INT", "42"},
	    {"LIBENVPP_TESTING1_FLOAT", "3.1415"},
	};

	const auto testing_env2 = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING2_INT", "24"},
	    {"LIBENVPP_TESTING2_FLOAT", "6.28318"},
	};

	const auto scoped_env1 = env::scoped_test_environment(testing_env1);
	const auto scoped_env2 = env::scoped_test_environment(testing_env2);

	{
		auto pre = env::prefix("LIBENVPP_TESTING1");
		const auto int_id = pre.register_variable<int>("INT");
		const auto float_id = pre.register_variable<float>("FLOAT");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		REQUIRE(parsed_and_validated_pre.ok());
		const auto int_val = parsed_and_validated_pre.get(int_id);
		REQUIRE(int_val.has_value());
		CHECK(*int_val == 42);
		const auto float_val = parsed_and_validated_pre.get(float_id);
		REQUIRE(float_val.has_value());
		CHECK(*float_val == 3.1415f);
	}

	{
		auto pre = env::prefix("LIBENVPP_TESTING2");
		const auto int_id = pre.register_variable<int>("INT");
		const auto float_id = pre.register_variable<float>("FLOAT");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		REQUIRE(parsed_and_validated_pre.ok());
		const auto int_val = parsed_and_validated_pre.get(int_id);
		REQUIRE(int_val.has_value());
		CHECK(*int_val == 24);
		const auto float_val = parsed_and_validated_pre.get(float_id);
		REQUIRE(float_val.has_value());
		CHECK(*float_val == 6.28318f);
	}
}

TEST_CASE("Duplicate test environment entries are detected", "[libenvpp_testing]")
{
	const auto testing_env1 = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING_INT", "42"},
	    {"LIBENVPP_TESTING_FLOAT", "3.1415"},
	};

	const auto testing_env2 = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING_INT", "24"},
	    {"LIBENVPP_TESTING_FLOAT", "6.28318"},
	};

	const auto scoped_env1 = env::scoped_test_environment(testing_env1);

	CHECK_THROWS_AS(env::scoped_test_environment(testing_env2), test_environment_error);
	CHECK_THROWS_WITH(env::scoped_test_environment(testing_env2),
	                  ContainsSubstring("'LIBENVPP_TESTING_INT'") || ContainsSubstring("'LIBENVPP_TESTING_FLOAT'"));
}

TEST_CASE("Global testing environment is modified correctly", "[libenvpp_testing]")
{
	CHECK(detail::g_testing_environment.empty());

	const auto testing_env1 = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING1_INT", "42"},
	    {"LIBENVPP_TESTING1_FLOAT", "3.1415"},
	};

	const auto testing_env2 = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING2_INT", "24"},
	    {"LIBENVPP_TESTING2_FLOAT", "6.28318"},
	};

	constexpr auto check_env1 = [](bool contains = true) {
		CHECK((detail::g_testing_environment.find("LIBENVPP_TESTING1_INT") != detail::g_testing_environment.end())
		      == contains);
		CHECK((detail::g_testing_environment.find("LIBENVPP_TESTING1_FLOAT") != detail::g_testing_environment.end())
		      == contains);
		if (contains) {
			CHECK(detail::g_testing_environment["LIBENVPP_TESTING1_INT"] == "42");
			CHECK(detail::g_testing_environment["LIBENVPP_TESTING1_FLOAT"] == "3.1415");
		}
	};

	constexpr auto check_env2 = [](bool contains = true) {
		CHECK((detail::g_testing_environment.find("LIBENVPP_TESTING2_INT") != detail::g_testing_environment.end())
		      == contains);
		CHECK((detail::g_testing_environment.find("LIBENVPP_TESTING2_FLOAT") != detail::g_testing_environment.end())
		      == contains);
		if (contains) {
			CHECK(detail::g_testing_environment["LIBENVPP_TESTING2_INT"] == "24");
			CHECK(detail::g_testing_environment["LIBENVPP_TESTING2_FLOAT"] == "6.28318");
		}
	};

	{
		const auto scoped_env1 = env::scoped_test_environment(testing_env1);

		CHECK(detail::g_testing_environment.size() == 2);
		check_env1(true);
		check_env2(false);

		{
			const auto scoped_env2 = env::scoped_test_environment(testing_env2);
			CHECK(detail::g_testing_environment.size() == 4);
			check_env1(true);
			check_env2(true);
		}

		CHECK(detail::g_testing_environment.size() == 2);
		check_env1(true);
		check_env2(false);
	}

	CHECK(detail::g_testing_environment.empty());
	check_env1(false);
	check_env2(false);
}

TEST_CASE("Global testing environment takes precedence over environment variables", "[libenvpp_testing]")
{
	const auto scoped_env_var = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_INT", "7"};

	{
		const auto scoped_env1 = env::scoped_test_environment(std::unordered_map<std::string, std::string>{
		    {"LIBENVPP_TESTING_INT", "42"},
		});

		auto pre = env::prefix("LIBENVPP_TESTING");
		const auto int_id = pre.register_variable<int>("INT");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		REQUIRE(parsed_and_validated_pre.ok());
		const auto int_val = parsed_and_validated_pre.get(int_id);
		REQUIRE(int_val.has_value());
		CHECK(*int_val == 42);
	}

	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto int_id = pre.register_variable<int>("INT");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE(parsed_and_validated_pre.ok());
	const auto int_val = parsed_and_validated_pre.get(int_id);
	REQUIRE(int_val.has_value());
	CHECK(*int_val == 7);
}

TEST_CASE("Global testing environment takes precedence over custom environment", "[libenvpp_testing]")
{
	const auto testing_env1 = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING_INT", "42"},
	};

	const auto testing_env2 = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING_INT", "7"},
	};

	{
		const auto scoped_env1 = env::scoped_test_environment(testing_env1);

		auto pre = env::prefix("LIBENVPP_TESTING");
		const auto int_id = pre.register_variable<int>("INT");
		auto parsed_and_validated_pre = pre.parse_and_validate(testing_env2);
		REQUIRE(parsed_and_validated_pre.ok());
		const auto int_val = parsed_and_validated_pre.get(int_id);
		REQUIRE(int_val.has_value());
		CHECK(*int_val == 42);
	}

	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto int_id = pre.register_variable<int>("INT");
	auto parsed_and_validated_pre = pre.parse_and_validate(testing_env2);
	REQUIRE(parsed_and_validated_pre.ok());
	const auto int_val = parsed_and_validated_pre.get(int_id);
	REQUIRE(int_val.has_value());
	CHECK(*int_val == 7);
}

} // namespace env
