#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <libenvpp_env.hpp>

namespace env::detail {

using Catch::Matchers::Equals;

TEST_CASE("Deleting environment variables", "[libenvpp_env]")
{
	constexpr auto test_var_name = "LIBENVPP_TESTING_DELETE";
	constexpr auto test_var_value = "42";

	set_environment_variable(test_var_name, test_var_value);
	delete_environment_variable(test_var_name);
	CHECK_FALSE(get_environment_variable(test_var_name).has_value());
}

TEST_CASE("Scoped setter", "[libenvpp_env]")
{
	constexpr auto test_var_name = "LIBENVPP_TESTING_SCOPED";
	constexpr auto test_var_value = "3.1415";

	{
		const auto _ = set_scoped_environment_variable{test_var_name, test_var_value};
		const auto var = get_environment_variable(test_var_name);
		REQUIRE(var.has_value());
		CHECK_THAT(*var, Equals(test_var_value));
	}

	CHECK_FALSE(get_environment_variable(test_var_name).has_value());
}

TEST_CASE("Setting environment variables", "[libenvpp_env]")
{
	constexpr auto test_var_name = "LIBENVPP_TESTING_SET";
	constexpr auto test_var_value = "Foo Bar Baz";

	set_environment_variable(test_var_name, test_var_value);
	const auto var = get_environment_variable(test_var_name);
	REQUIRE(var.has_value());
	CHECK_THAT(*var, Equals(test_var_value));

	delete_environment_variable(test_var_name);
}

TEST_CASE("Overwriting environment variables", "[libenvpp_env]")
{
	constexpr auto test_var_name = "LIBENVPP_TESTING_OVERWRITE";
	constexpr auto test_var_value = "Foo";
	constexpr auto overwrite_value = "Bar";

	set_environment_variable(test_var_name, test_var_value);
	const auto var = get_environment_variable(test_var_name);
	REQUIRE(var.has_value());
	CHECK_THAT(*var, Equals(test_var_value));

	set_environment_variable(test_var_name, overwrite_value);
	const auto overwritten_var = get_environment_variable(test_var_name);
	REQUIRE(overwritten_var.has_value());
	CHECK_THAT(*overwritten_var, Equals(overwrite_value));

	delete_environment_variable(test_var_name);
}

TEST_CASE("Getting environment variables", "[libenvpp_env]")
{
	constexpr auto test_var_name = "LIBENVPP_TESTING_GET";
	constexpr auto test_var_value = "Hello World!";

	const auto _ = set_scoped_environment_variable{test_var_name, test_var_value};
	const auto test_var = get_environment_variable(test_var_name);
	REQUIRE(test_var.has_value());
	CHECK_THAT(*test_var, Equals(test_var_value));
}

TEST_CASE("Getting entire environment", "[libenvpp_env]")
{
	const auto environment = get_environment();
	CHECK_FALSE(environment.empty());
}

TEST_CASE("Environment contains set variables", "[libenvpp_env]")
{
	constexpr auto test_var_name = "LIBENVPP_TESTING_ENVIRONMENT";
	constexpr auto test_var_value = "value";

	const auto _ = set_scoped_environment_variable{test_var_name, test_var_value};

	const auto environment = get_environment();
	REQUIRE_FALSE(environment.empty());

	REQUIRE(environment.find(test_var_name) != environment.end());
	CHECK_THAT(environment.at(test_var_name), Equals(test_var_value));
}

} // namespace env::detail
