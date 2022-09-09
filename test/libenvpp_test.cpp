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

TEST_CASE("Unset environment variables", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	SECTION("Unset optional variable")
	{
		auto pre = env::prefix(prefix_name);
		const auto var_id = pre.register_variable<int>("UNSET");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto var_opt_val = parsed_and_validated_pre.get(var_id);
		CHECK_FALSE(var_opt_val.has_value());
		const auto var_default_val = parsed_and_validated_pre.get_or(var_id, -1);
		CHECK(var_default_val == -1);
	}

	SECTION("Unset required variable")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto var_id = pre.register_required_variable<int>("UNSET");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(), ContainsSubstring("error")
		                                                         && ContainsSubstring(prefix_name)
		                                                         && ContainsSubstring("'UNSET' not set"));
		// Should not compile
		// const auto val = parsed_and_validated_pre.get_or(var_id, -1);
	}
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
	SECTION("Optional variable")
	{
		auto pre = env::prefix("LIBENVPP_TESTING");
		[[maybe_unused]] const auto int_id = pre.register_variable<int>("HINT");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		REQUIRE_FALSE(parsed_and_validated_pre.ok());

		CHECK_THAT(parsed_and_validated_pre.warning_message(),
		           ContainsSubstring("'LIBENVPP_TESTING_INT' set")
		               && ContainsSubstring("did you mean 'LIBENVPP_TESTING_HINT'"));
	}

	SECTION("Required variable")
	{
		auto pre = env::prefix("LIBENVPP_TESTING");
		[[maybe_unused]] const auto int_id = pre.register_required_variable<int>("HINT");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		REQUIRE_FALSE(parsed_and_validated_pre.ok());

		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           ContainsSubstring("'LIBENVPP_TESTING_INT' set")
		               && ContainsSubstring("did you mean 'LIBENVPP_TESTING_HINT'"));
	}
}

TEST_CASE("Typo detection does not trigger on already consumed variables", "[libenvpp]")
{
	const auto foo_var = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_FOO", "BAR"};
	const auto fou_var = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_FOU", "BAR"};

	auto pre = env::prefix("LIBENVPP_TESTING");
	[[maybe_unused]] const auto foo_id = pre.register_variable<std::string>("FOO");
	[[maybe_unused]] const auto fuu_id = pre.register_variable<std::string>("FUU");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE_FALSE(parsed_and_validated_pre.ok());

	CHECK_THAT(parsed_and_validated_pre.warning_message(),
	           ContainsSubstring("'LIBENVPP_TESTING_FOU' set")
	               && ContainsSubstring("did you mean 'LIBENVPP_TESTING_FUU'"));
}

TEST_CASE("Unused variable with same prefix", "[libenvpp]")
{
	SECTION("Actual prefix")
	{
		const auto _ = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_FOO", "FOO"};

		auto pre = env::prefix("LIBENVPP_TESTING");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		REQUIRE_FALSE(parsed_and_validated_pre.ok());

		CHECK_THAT(parsed_and_validated_pre.warning_message(), ContainsSubstring("LIBENVPP_TESTING_FOO"));
	}

	SECTION("Prefix is a substring, but not at the beginning")
	{
		const auto _ = detail::set_scoped_environment_variable{"FOO_LIBENVPP_TESTING", "FOO"};

		auto pre = env::prefix("LIBENVPP_TESTING");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
	}
}

TEST_CASE("Set for testing", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto int_id = pre.register_variable<int>("INT");
	const auto float_id = pre.register_required_variable<float>("FLOAT");
	pre.set_for_testing(int_id, 42);
	pre.set_for_testing(float_id, 3.1415f);
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE(parsed_and_validated_pre.ok());
	const auto int_val = parsed_and_validated_pre.get(int_id);
	REQUIRE(int_val.has_value());
	CHECK(*int_val == 42);
	const auto float_val = parsed_and_validated_pre.get(float_id);
	CHECK(float_val == 3.1415f);
}

TEST_CASE("Help message", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	[[maybe_unused]] const auto int_id = pre.register_variable<int>("INTEGER");
	[[maybe_unused]] const auto float_id = pre.register_required_variable<float>("FLOAT");
	const auto pre_help_message = pre.help_message();
	auto parsed_pre = pre.parse_and_validate();
	const auto parsed_help_message = parsed_pre.help_message();
	CHECK_THAT(pre_help_message, Equals(parsed_help_message));
	CHECK_THAT(pre_help_message, ContainsSubstring("LIBENVPP_TESTING") && ContainsSubstring("2")
	                                 && ContainsSubstring("INTEGER optional") && ContainsSubstring("FLOAT required"));
}

TEST_CASE("Parser errors", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	const auto _ = detail::set_scoped_environment_variable{prefix_name + std::string("_ENV_VAR"), "FOO"};

	SECTION("Built-in type")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto int_id = pre.register_variable<int>("ENV_VAR");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           ContainsSubstring(prefix_name) && ContainsSubstring("Parser error") && ContainsSubstring("ENV_VAR"));
	}

	SECTION("User-defined type with specialized default parser")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto option_id = pre.register_variable<testing_option>("ENV_VAR");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           ContainsSubstring(prefix_name) && ContainsSubstring("Parser error") && ContainsSubstring("ENV_VAR"));
	}

	SECTION("User-defined type with custom parser")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto var_id = pre.register_variable<int>(
		    "ENV_VAR", [](const std::string_view) -> int { throw parser_error{"Unparseable"}; });
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           ContainsSubstring(prefix_name) && ContainsSubstring("Parser error")
		               && ContainsSubstring("Unparseable") && ContainsSubstring("ENV_VAR"));
	}
}

struct unvalidatable_type {
	unvalidatable_type(const std::string_view) {}
};

template <>
struct default_validator<unvalidatable_type> {
	void operator()(const unvalidatable_type&) const { throw validation_error{"Unvalidatable"}; }
};

TEST_CASE("Validation errors", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	const auto _ = detail::set_scoped_environment_variable{prefix_name + std::string("_ENV_VAR"), "FOO"};

	SECTION("User-defined type with specialized default validator")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto var_id = pre.register_variable<unvalidatable_type>("ENV_VAR");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           ContainsSubstring(prefix_name) && ContainsSubstring("Validation error")
		               && ContainsSubstring("Unvalidatable") && ContainsSubstring("ENV_VAR"));
	}

	SECTION("User-defined type with custom validator")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto var_id = pre.register_variable<int>(
		    "ENV_VAR", [](const std::string_view) -> int { throw validation_error{"Unvalidatable"}; });
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           ContainsSubstring(prefix_name) && ContainsSubstring("Validation error")
		               && ContainsSubstring("Unvalidatable") && ContainsSubstring("ENV_VAR"));
	}
}

TEST_CASE_METHOD(int_var_fixture, "Range environment variables", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	SECTION("Optional range")
	{
		auto pre = env::prefix(prefix_name);
		const auto range_id = pre.register_range<int>("INT", 0, 100);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto range_val = parsed_and_validated_pre.get(range_id);
		REQUIRE(range_val.has_value());
		CHECK(*range_val == 42);
	}

	SECTION("Required range")
	{
		auto pre = env::prefix(prefix_name);
		const auto range_id = pre.register_required_range<int>("INT", 0, 100);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto range_val = parsed_and_validated_pre.get(range_id);
		CHECK(range_val == 42);
	}
}

TEST_CASE_METHOD(int_var_fixture, "Invalid range environment variables", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	SECTION("Invalid optional range")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto range_id = pre.register_range<int>("INT", 100, 200);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           ContainsSubstring("Range error") && ContainsSubstring(prefix_name) && ContainsSubstring("INT")
		               && ContainsSubstring("42") && ContainsSubstring("[100, 200]"));
	}

	SECTION("Required range")
	{
		auto pre = env::prefix(prefix_name);
		const auto range_id = pre.register_required_range<int>("INT", 0, 100);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto range_val = parsed_and_validated_pre.get(range_id);
		CHECK(range_val == 42);
	}
}

TEST_CASE("Unset range environment variables", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	SECTION("Unset optional range")
	{
		auto pre = env::prefix(prefix_name);
		const auto range_id = pre.register_range<int>("UNSET", 0, 100);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto range_opt_val = parsed_and_validated_pre.get(range_id);
		CHECK_FALSE(range_opt_val.has_value());
		const auto range_default_val = parsed_and_validated_pre.get_or(range_id, -1);
		// Default value outside of range is allowed to make special case handling possible
		CHECK(range_default_val == -1);
	}

	SECTION("Unset required range")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto range_id = pre.register_required_range<int>("UNSET", 0, 100);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(), ContainsSubstring("error")
		                                                         && ContainsSubstring(prefix_name)
		                                                         && ContainsSubstring("'UNSET' not set"));
	}
}

} // namespace env
