#include <iostream>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <libenvpp.hpp>
#include <libenvpp_env.hpp>

namespace env {

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::Equals;

class int_var_fixture {
  public:
	int_var_fixture() : m_var("LIBENVPP_TESTING_INT", "42") {}

  private:
	detail::set_scoped_environment_variable m_var;
};

class float_var_fixture {
  public:
	float_var_fixture() : m_var("LIBENVPP_TESTING_FLOAT", "3.1415") {}

  private:
	detail::set_scoped_environment_variable m_var;
};

class string_var_fixture {
  public:
	string_var_fixture() : m_var("LIBENVPP_TESTING_STRING", "Hello World") {}

  private:
	detail::set_scoped_environment_variable m_var;
};

enum class testing_option {
	FIRST_OPTION,
	SECOND_OPTION,
	THIRD_OPTION,
};

template <>
struct default_parser<testing_option> {
	[[nodiscard]] testing_option operator()(const std::string_view str) const
	{
		testing_option value;
		if (str == "FIRST_OPTION") {
			value = testing_option::FIRST_OPTION;
		} else if (str == "SECOND_OPTION") {
			value = testing_option::SECOND_OPTION;
		} else if (str == "THIRD_OPTION") {
			value = testing_option::THIRD_OPTION;
		} else {
			throw parser_error{fmt::format("Cannot construct testing_option from '{}'", str)};
		}
		return value;
	}
};

class option_var_fixture {
  public:
	option_var_fixture() : m_var("LIBENVPP_TESTING_OPTION", "SECOND_OPTION") {}

  private:
	detail::set_scoped_environment_variable m_var;
};

TEST_CASE_METHOD(int_var_fixture, "Retrieving integer environment variable", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto int_id = pre.register_variable<int>("INT");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE(parsed_and_validated_pre.ok());
	const auto int_val = parsed_and_validated_pre.get(int_id);
	REQUIRE(int_val.has_value());
	CHECK(*int_val == 42);
}

TEST_CASE_METHOD(float_var_fixture, "Retrieving float environment variable", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto float_id = pre.register_variable<float>("FLOAT");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE(parsed_and_validated_pre.ok());
	const auto float_val = parsed_and_validated_pre.get(float_id);
	REQUIRE(float_val.has_value());
	CHECK(*float_val == 3.1415f);
}

TEST_CASE_METHOD(string_var_fixture, "Retrieving string environment variable", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto string_id = pre.register_variable<std::string>("STRING");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE(parsed_and_validated_pre.ok());
	const auto string_val = parsed_and_validated_pre.get(string_id);
	REQUIRE(string_val.has_value());
	CHECK_THAT(*string_val, Equals("Hello World"));
}

TEST_CASE_METHOD(option_var_fixture, "Retrieving option environment variable", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto option_id = pre.register_variable<testing_option>("OPTION");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE(parsed_and_validated_pre.ok());
	const auto option_val = parsed_and_validated_pre.get(option_id);
	REQUIRE(option_val.has_value());
	CHECK(*option_val == testing_option::SECOND_OPTION);
}

TEST_CASE("Retrieving errors", "[libenvpp]")
{
	const auto prefix_name = "PREFIX";
	auto pre = env::prefix(prefix_name);
	const auto foo_name = "FOO";
	const auto foo_id = pre.register_required_variable<int>(foo_name);
	auto parsed_pre = pre.parse_and_validate();
	REQUIRE_FALSE(parsed_pre.ok());

	SECTION("Formatted error message")
	{
		CHECK_THAT(parsed_pre.error_message(), ContainsSubstring("error") && ContainsSubstring(prefix_name)
		                                           && ContainsSubstring(foo_name) && ContainsSubstring("not set"));
	}

	SECTION("Relating error to ID")
	{
		for (const auto& err : parsed_pre.errors()) {
			const auto err_id = err.get_id();
			CHECK(err_id == foo_id);
			CHECK_FALSE(err_id != foo_id);
			CHECK(foo_id == err_id);
			CHECK_FALSE(foo_id != err_id);
		}
	}

	SECTION("Relating error to name")
	{
		for (const auto& err : parsed_pre.errors()) {
			const auto err_name = err.get_name();
			CHECK_THAT(err_name, Equals(foo_name));
		}
	}
}

TEST_CASE_METHOD(int_var_fixture, "Typo detection using edit distance", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	[[maybe_unused]] const auto int_id = pre.register_required_variable<int>("HINT");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE_FALSE(parsed_and_validated_pre.ok());

	CHECK_THAT(parsed_and_validated_pre.error_message(),
	           ContainsSubstring("INT") && ContainsSubstring("did you mean") && ContainsSubstring("HINT"));
}

TEST_CASE("Unused variable with same prefix", "[libenvpp]")
{
	const auto scoped_env_var = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_FOO", "FOO"};

	auto pre = env::prefix("LIBENVPP_TESTING");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE_FALSE(parsed_and_validated_pre.ok());

	CHECK_THAT(parsed_and_validated_pre.warning_message(), ContainsSubstring("LIBENVPP_TESTING_FOO"));
}

} // namespace env
