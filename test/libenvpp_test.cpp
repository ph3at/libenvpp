#include <limits>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <fmt/core.h>

#include <libenvpp/detail/environment.hpp>
#include <libenvpp/env.hpp>

namespace env {

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::Equals;
using Catch::Matchers::IsEmpty;
using Catch::Matchers::StartsWith;

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

struct user_parsable_type {
	std::string value;
};

template <>
struct default_parser<user_parsable_type> {
	[[nodiscard]] user_parsable_type operator()(const std::string_view str) const { return {std::string(str)}; }
};

TEST_CASE_METHOD(string_var_fixture, "User-defined type with specialized default parser", "[libenvpp]")
{
	SECTION("Optional")
	{
		auto pre = env::prefix("LIBENVPP_TESTING");
		const auto var_id = pre.register_variable<user_parsable_type>("STRING");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto var_opt_val = parsed_and_validated_pre.get(var_id);
		REQUIRE(var_opt_val.has_value());
		CHECK_THAT(var_opt_val->value, Equals("Hello World"));
	}

	SECTION("Required")
	{
		auto pre = env::prefix("LIBENVPP_TESTING");
		const auto var_id = pre.register_required_variable<user_parsable_type>("STRING");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto var_val = parsed_and_validated_pre.get(var_id);
		CHECK_THAT(var_val.value, Equals("Hello World"));
	}
}

struct user_validatable_type {
	user_validatable_type(const std::string_view str) : value(str) {}
	std::string value;
};

template <>
struct default_validator<user_validatable_type> {
	void operator()(const user_validatable_type& value) const { CHECK_THAT(value.value, Equals("Hello World")); }
};

TEST_CASE_METHOD(string_var_fixture, "User-defined type with specialized default validator", "[libenvpp]")
{
	SECTION("Optional")
	{
		auto pre = env::prefix("LIBENVPP_TESTING");
		const auto var_id = pre.register_variable<user_validatable_type>("STRING");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto var_opt_val = parsed_and_validated_pre.get(var_id);
		REQUIRE(var_opt_val.has_value());
		CHECK_THAT(var_opt_val->value, Equals("Hello World"));
	}

	SECTION("Required")
	{
		auto pre = env::prefix("LIBENVPP_TESTING");
		const auto var_id = pre.register_required_variable<user_validatable_type>("STRING");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto var_val = parsed_and_validated_pre.get(var_id);
		CHECK_THAT(var_val.value, Equals("Hello World"));
	}
}

std::string string_parser_and_validator_fn(const std::string_view str)
{
	const auto val = std::string(str);
	CHECK_THAT(val, Equals("Hello World"));
	return val;
}

TEST_CASE_METHOD(string_var_fixture, "User-defined parser and validator function", "[libenvpp]")
{
	SECTION("Optional")
	{
		auto pre = env::prefix("LIBENVPP_TESTING");
		const auto var_id = pre.register_variable<std::string>("STRING", string_parser_and_validator_fn);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto var_opt_val = parsed_and_validated_pre.get(var_id);
		REQUIRE(var_opt_val.has_value());
		CHECK_THAT(*var_opt_val, Equals("Hello World"));
	}

	SECTION("Required")
	{
		auto pre = env::prefix("LIBENVPP_TESTING");
		const auto var_id = pre.register_required_variable<std::string>("STRING", string_parser_and_validator_fn);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto var_val = parsed_and_validated_pre.get(var_id);
		CHECK_THAT(var_val, Equals("Hello World"));
	}
}

TEST_CASE_METHOD(string_var_fixture, "User-defined parser and validator lambda", "[libenvpp]")
{
	constexpr auto string_parser_and_validator = [](const std::string_view str) {
		return string_parser_and_validator_fn(str);
	};

	SECTION("Optional")
	{
		auto pre = env::prefix("LIBENVPP_TESTING");
		const auto var_id = pre.register_variable<std::string>("STRING", string_parser_and_validator);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto var_opt_val = parsed_and_validated_pre.get(var_id);
		REQUIRE(var_opt_val.has_value());
		CHECK_THAT(*var_opt_val, Equals("Hello World"));
	}

	SECTION("Required")
	{
		auto pre = env::prefix("LIBENVPP_TESTING");
		const auto var_id = pre.register_required_variable<std::string>("STRING", string_parser_and_validator);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto var_val = parsed_and_validated_pre.get(var_id);
		CHECK_THAT(var_val, Equals("Hello World"));
	}
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
		const auto var_id = pre.register_required_variable<int>("UNSET");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           StartsWith("Error") && ContainsSubstring(prefix_name)
		               && ContainsSubstring("'LIBENVPP_TESTING_UNSET' not set"));
		CHECK_THROWS_AS(parsed_and_validated_pre.get(var_id), value_error);

		// Should not compile
		// const auto val = parsed_and_validated_pre.get_or(var_id, -1);
	}
}

TEST_CASE("Retrieving errors", "[libenvpp]")
{
	const auto prefix_name = std::string("PREFIX");
	auto pre = env::prefix(prefix_name);
	const auto foo_name = std::string("FOO");
	const auto foo_id = pre.register_required_variable<int>(foo_name);
	auto parsed_pre = pre.parse_and_validate();
	REQUIRE_FALSE(parsed_pre.ok());

	SECTION("Formatted error message")
	{
		CHECK_THAT(parsed_pre.error_message(), StartsWith("Error") && ContainsSubstring(prefix_name)
		                                           && ContainsSubstring("'" + prefix_name + "_" + foo_name + "'")
		                                           && ContainsSubstring("not set"));
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
			CHECK_THAT(err_name, Equals(prefix_name + "_" + foo_name));
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
		           StartsWith("Warning") && ContainsSubstring("'LIBENVPP_TESTING_INT' set")
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

TEST_CASE_METHOD(int_var_fixture, "Typos in prefix are detected", "[libenvpp]")
{
	auto pre = env::prefix("LIBVENPP_TESTING");
	[[maybe_unused]] const auto int_id = pre.register_variable<int>("INT");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	REQUIRE_FALSE(parsed_and_validated_pre.ok());

	CHECK_THAT(parsed_and_validated_pre.warning_message(),
	           StartsWith("Warning") && ContainsSubstring("'LIBENVPP_TESTING_INT' set")
	               && ContainsSubstring("did you mean 'LIBVENPP_TESTING_INT'"));
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
	           StartsWith("Warning") && ContainsSubstring("'LIBENVPP_TESTING_FOU' set")
	               && ContainsSubstring("did you mean 'LIBENVPP_TESTING_FUU'"));
}

TEST_CASE("Custom edit distance cutoff value", "[libenvpp]")
{
	SECTION("Typo detection disabled")
	{
		const auto _ = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_FOO", "BAR"};

		auto pre = env::prefix("LIBENVPP_TESTING", edit_distance{0});
		[[maybe_unused]] const auto env_var_id = pre.register_variable<std::string>("FUO");
		const auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK_THAT(parsed_and_validated_pre.warning_message(),
		           StartsWith("Warning") && ContainsSubstring("'LIBENVPP_TESTING_FOO' specified but unused")
		               && !ContainsSubstring("did you mean"));
	}

	SECTION("Edit distance of 1")
	{
		const auto abc_var = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_ABC", "BAR"};
		const auto def_var = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_DEF", "BAR"};

		auto pre = env::prefix("LIBENVPP_TESTING", edit_distance{1});
		[[maybe_unused]] const auto abz_id = pre.register_variable<std::string>("ABZ");
		[[maybe_unused]] const auto dyz_id = pre.register_variable<std::string>("DYZ");
		const auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		REQUIRE(parsed_and_validated_pre.warnings().size() == 2);
		CHECK_THAT(parsed_and_validated_pre.warnings()[0].what(),
		           ContainsSubstring("'LIBENVPP_TESTING_ABC' set")
		               && ContainsSubstring("did you mean 'LIBENVPP_TESTING_ABZ'"));
		CHECK_THAT(parsed_and_validated_pre.warnings()[1].what(),
		           ContainsSubstring("'LIBENVPP_TESTING_DEF' specified but unused")
		               && !ContainsSubstring("did you mean"));
	}
}

TEST_CASE("Environment variable name length is taken into account for typo detection", "[libenvpp]")
{
	SECTION("Variables with length <= 3 are not typo checked")
	{
		const auto _ = detail::set_scoped_environment_variable{"P_F", "BAR"};

		auto pre = env::prefix("P");
		[[maybe_unused]] const auto env_var_id = pre.register_variable<std::string>("V");
		const auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK_THAT(parsed_and_validated_pre.warning_message(), StartsWith("Warning")
		                                                           && ContainsSubstring("'P_F' specified but unused")
		                                                           && !ContainsSubstring("did you mean"));
	}

	SECTION("Variables with length <= 6 can at most have 1 typo")
	{
		const auto ab_var = detail::set_scoped_environment_variable{"PRE_AB", "BAR"};
		const auto cd_var = detail::set_scoped_environment_variable{"PRE_CD", "BAR"};

		auto pre = env::prefix("PRE");
		[[maybe_unused]] const auto az_id = pre.register_variable<std::string>("AZ");
		[[maybe_unused]] const auto yz_id = pre.register_variable<std::string>("YZ");
		const auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		REQUIRE(parsed_and_validated_pre.warnings().size() == 2);
		CHECK_THAT(parsed_and_validated_pre.warnings()[0].what(),
		           ContainsSubstring("'PRE_AB' set") && ContainsSubstring("did you mean 'PRE_AZ'"));
		CHECK_THAT(parsed_and_validated_pre.warnings()[1].what(),
		           ContainsSubstring("'PRE_CD' specified but unused") && !ContainsSubstring("did you mean"));
	}

	SECTION("Variables with length <= 9 can at most have 2 typos")
	{
		const auto abc_var = detail::set_scoped_environment_variable{"PREFI_ABC", "BAR"};
		const auto def_var = detail::set_scoped_environment_variable{"PREFI_DEF", "BAR"};

		auto pre = env::prefix("PREFI");
		[[maybe_unused]] const auto ayz_id = pre.register_variable<std::string>("AYZ");
		[[maybe_unused]] const auto xyz_id = pre.register_variable<std::string>("XYZ");
		const auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		REQUIRE(parsed_and_validated_pre.warnings().size() == 2);
		CHECK_THAT(parsed_and_validated_pre.warnings()[0].what(),
		           ContainsSubstring("'PREFI_ABC' set") && ContainsSubstring("did you mean 'PREFI_AYZ'"));
		CHECK_THAT(parsed_and_validated_pre.warnings()[1].what(),
		           ContainsSubstring("'PREFI_DEF' specified but unused") && !ContainsSubstring("did you mean"));
	}

	SECTION("Variables with length > 9 can at most have 3 typos")
	{
		const auto abcd_var = detail::set_scoped_environment_variable{"PREFI_ABCD", "BAR"};
		const auto efgh_var = detail::set_scoped_environment_variable{"PREFI_EFGH", "BAR"};

		auto pre = env::prefix("PREFI");
		[[maybe_unused]] const auto axyz_id = pre.register_variable<std::string>("AXYZ");
		[[maybe_unused]] const auto wxyz_id = pre.register_variable<std::string>("WXYZ");
		const auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		REQUIRE(parsed_and_validated_pre.warnings().size() == 2);
		CHECK_THAT(parsed_and_validated_pre.warnings()[0].what(),
		           ContainsSubstring("'PREFI_ABCD' set") && ContainsSubstring("did you mean 'PREFI_AXYZ'"));
		CHECK_THAT(parsed_and_validated_pre.warnings()[1].what(),
		           ContainsSubstring("'PREFI_EFGH' specified but unused") && !ContainsSubstring("did you mean"));
	}
}

TEST_CASE("Unused variable with same prefix", "[libenvpp]")
{
	SECTION("Actual prefix")
	{
		const auto _ = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_FOO", "FOO"};

		auto pre = env::prefix("LIBENVPP_TESTING");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		REQUIRE_FALSE(parsed_and_validated_pre.ok());

		CHECK_THAT(parsed_and_validated_pre.warning_message(), ContainsSubstring("'LIBENVPP_TESTING_FOO'"));
	}

	SECTION("Prefix is a substring, but not at the beginning")
	{
		const auto _ = detail::set_scoped_environment_variable{"FOO_LIBENVPP_TESTING", "FOO"};

		auto pre = env::prefix("LIBENVPP_TESTING");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
	}

	SECTION("Prefix check considers '_' as part of the prefix")
	{
		const auto foo_var = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_FOO", "FOO"};
		const auto bar_var = detail::set_scoped_environment_variable{"LIBENVPP_TESTINGBAR", "BAR"};

		auto pre = env::prefix("LIBENVPP_TESTING");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.warning_message(),
		           ContainsSubstring("'LIBENVPP_TESTING_FOO'") && !ContainsSubstring("'LIBENVPP_TESTINGBAR'"));
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

	SECTION("Empty environment")
	{
		const auto help_message = pre.help_message();
		CHECK_THAT(help_message,
		           ContainsSubstring("LIBENVPP_TESTING") && ContainsSubstring("no supported environment variables"));
	}

	SECTION("Non-empty environment")
	{
		[[maybe_unused]] const auto int_id = pre.register_variable<int>("INTEGER");
		[[maybe_unused]] const auto float_id = pre.register_required_variable<float>("FLOAT");
		const auto pre_help_message = pre.help_message();
		auto parsed_pre = pre.parse_and_validate();
		const auto parsed_help_message = parsed_pre.help_message();
		CHECK_THAT(pre_help_message, Equals(parsed_help_message));
		CHECK_THAT(pre_help_message, ContainsSubstring("LIBENVPP_TESTING") && ContainsSubstring("2")
		                                 && ContainsSubstring("'LIBENVPP_TESTING_INTEGER' optional")
		                                 && ContainsSubstring("'LIBENVPP_TESTING_FLOAT' required"));
	}
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
		CHECK_THAT(parsed_and_validated_pre.error_message(), ContainsSubstring(prefix_name)
		                                                         && ContainsSubstring("Parser error")
		                                                         && ContainsSubstring("'LIBENVPP_TESTING_ENV_VAR'"));
	}

	SECTION("User-defined type with specialized default parser")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto option_id = pre.register_variable<testing_option>("ENV_VAR");
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(), ContainsSubstring(prefix_name)
		                                                         && ContainsSubstring("Parser error")
		                                                         && ContainsSubstring("'LIBENVPP_TESTING_ENV_VAR'"));
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
		               && ContainsSubstring("Unparseable") && ContainsSubstring("'LIBENVPP_TESTING_ENV_VAR'"));
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
		               && ContainsSubstring("Unvalidatable") && ContainsSubstring("'LIBENVPP_TESTING_ENV_VAR'"));
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
		               && ContainsSubstring("Unvalidatable") && ContainsSubstring("'LIBENVPP_TESTING_ENV_VAR'"));
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

	SECTION("Single element range")
	{
		auto pre = env::prefix(prefix_name);
		const auto range_id = pre.register_required_range<int>("INT", 42, 42);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto range_val = parsed_and_validated_pre.get(range_id);
		CHECK(range_val == 42);
	}
}

TEST_CASE("Invalid range registered", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	CHECK_THROWS_AS(pre.register_range<int>("INT", 200, 100), invalid_range);
	CHECK_THROWS_WITH(pre.register_range<int>("INT", 200, 100), ContainsSubstring("'LIBENVPP_TESTING_INT'"));
	CHECK_THROWS_AS(pre.register_required_range<int>("INT", 200, 100), invalid_range);
	CHECK_THROWS_WITH(pre.register_required_range<int>("INT", 200, 100), ContainsSubstring("'LIBENVPP_TESTING_INT'"));
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
		           ContainsSubstring("Range error") && ContainsSubstring(prefix_name)
		               && ContainsSubstring("'LIBENVPP_TESTING_INT'") && ContainsSubstring("42")
		               && ContainsSubstring("[100, 200]"));
	}

	SECTION("Invalid required range")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto range_id = pre.register_required_range<int>("INT", 100, 200);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           ContainsSubstring("Range error") && ContainsSubstring(prefix_name)
		               && ContainsSubstring("'LIBENVPP_TESTING_INT'") && ContainsSubstring("42")
		               && ContainsSubstring("[100, 200]"));
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
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           StartsWith("Error") && ContainsSubstring(prefix_name)
		               && ContainsSubstring("'LIBENVPP_TESTING_UNSET' not set"));
	}
}

TEST_CASE("Using numeric limits as range", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	SECTION("Lower bound")
	{
		const auto test_num =
		    GENERATE(std::numeric_limits<int>::lowest(), std::numeric_limits<int>::lowest() + 1, 0, 100);
		const auto _ =
		    detail::set_scoped_environment_variable{prefix_name + std::string("_ENV_VAR"), std::to_string(test_num)};

		auto pre = env::prefix(prefix_name);
		const auto range_id = pre.register_range<int>("ENV_VAR", std::numeric_limits<int>::lowest(), 100);
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto range_val = parsed_and_validated_pre.get(range_id);
		REQUIRE(range_val.has_value());
		CHECK(*range_val == test_num);
	}

	SECTION("Upper bound")
	{
		const auto test_num = GENERATE(std::numeric_limits<int>::max(), std::numeric_limits<int>::max() - 1, 0, -100);
		const auto _ =
		    detail::set_scoped_environment_variable{prefix_name + std::string("_ENV_VAR"), std::to_string(test_num)};

		auto pre = env::prefix(prefix_name);
		const auto range_id = pre.register_range<int>("ENV_VAR", -100, std::numeric_limits<int>::max());
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto range_val = parsed_and_validated_pre.get(range_id);
		REQUIRE(range_val.has_value());
		CHECK(*range_val == test_num);
	}
}

TEST_CASE_METHOD(int_var_fixture, "Option environment variables", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	SECTION("Optional option")
	{
		auto pre = env::prefix(prefix_name);
		const auto option_id = pre.register_option<int>("INT", {41, 42, 43});
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto option_val = parsed_and_validated_pre.get(option_id);
		REQUIRE(option_val.has_value());
		CHECK(*option_val == 42);
	}

	SECTION("Required option")
	{
		auto pre = env::prefix(prefix_name);
		const auto option_id = pre.register_required_option<int>("INT", {41, 42, 43});
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto option_val = parsed_and_validated_pre.get(option_id);
		CHECK(option_val == 42);
	}
}

TEST_CASE("Empty option", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	CHECK_THROWS_AS(pre.register_option<int>("INT", {}), empty_option);
	CHECK_THROWS_WITH(pre.register_option<int>("INT", {}), ContainsSubstring("'LIBENVPP_TESTING_INT'"));
	CHECK_THROWS_AS(pre.register_required_option<int>("INT", {}), empty_option);
	CHECK_THROWS_WITH(pre.register_required_option<int>("INT", {}), ContainsSubstring("'LIBENVPP_TESTING_INT'"));
}

TEST_CASE("Duplicate option", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	CHECK_THROWS_AS(pre.register_option<int>("INT", {1, 1}), duplicate_option);
	CHECK_THROWS_WITH(pre.register_option<int>("INT", {1, 1}), ContainsSubstring("'LIBENVPP_TESTING_INT'"));
	CHECK_THROWS_AS(pre.register_required_option<int>("INT", {1, 1}), duplicate_option);
	CHECK_THROWS_WITH(pre.register_required_option<int>("INT", {1, 1}), ContainsSubstring("'LIBENVPP_TESTING_INT'"));
}

TEST_CASE_METHOD(int_var_fixture, "Invalid option environment variables", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	SECTION("Invalid optional option")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto option_id = pre.register_option<int>("INT", {1, 2, 3});
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           ContainsSubstring("Option error") && ContainsSubstring("Unrecognized option")
		               && ContainsSubstring(prefix_name) && ContainsSubstring("'LIBENVPP_TESTING_INT'")
		               && ContainsSubstring("42"));
	}

	SECTION("Invalid required option")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto option_id = pre.register_required_option<int>("INT", {1, 2, 3});
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           ContainsSubstring("Option error") && ContainsSubstring("Unrecognized option")
		               && ContainsSubstring(prefix_name) && ContainsSubstring("'LIBENVPP_TESTING_INT'")
		               && ContainsSubstring("42"));
	}
}

TEST_CASE("Unset option environment variables", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	SECTION("Unset optional option")
	{
		auto pre = env::prefix(prefix_name);
		const auto option_id = pre.register_option<int>("UNSET", {1});
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK(parsed_and_validated_pre.ok());
		const auto option_opt_val = parsed_and_validated_pre.get(option_id);
		CHECK_FALSE(option_opt_val.has_value());
		const auto option_default_val = parsed_and_validated_pre.get_or(option_id, -1);
		// Default value outside of available options is allowed to make special case handling possible
		CHECK(option_default_val == -1);
	}

	SECTION("Unset required option")
	{
		auto pre = env::prefix(prefix_name);
		[[maybe_unused]] const auto option_id = pre.register_required_option<int>("UNSET", {1});
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		CHECK(parsed_and_validated_pre.errors().size() == 1);
		CHECK_THAT(parsed_and_validated_pre.error_message(),
		           StartsWith("Error") && ContainsSubstring(prefix_name)
		               && ContainsSubstring("'LIBENVPP_TESTING_UNSET' not set"));
	}
}

TEST_CASE("Custom environment", "[libenvpp]")
{
	auto custom_env = std::unordered_map<std::string, std::string>{
	    {"LIBENVPP_TESTING_INT", "42"},
	    {"LIBENVPP_TESTING_FLOAT", "3.1415"},
	    {"LIBENVPP_TESTING_STRING", "Hello World"},
	};

	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto int_id = pre.register_required_variable<int>("INT");
	const auto float_id = pre.register_required_variable<float>("FLOAT");
	const auto string_id = pre.register_required_variable<std::string>("STRING");

	SECTION("Found in custom env")
	{
		auto parsed_and_validated_pre = pre.parse_and_validate(custom_env);
		CHECK(parsed_and_validated_pre.ok());
		const auto int_val = parsed_and_validated_pre.get(int_id);
		const auto float_val = parsed_and_validated_pre.get(float_id);
		const auto string_val = parsed_and_validated_pre.get(string_id);
		CHECK(int_val == 42);
		CHECK(float_val == 3.1415f);
		CHECK_THAT(string_val, Equals("Hello World"));
	}

	SECTION("Not found in system env")
	{
		auto parsed_and_validated_pre = pre.parse_and_validate();
		CHECK_FALSE(parsed_and_validated_pre.ok());
		CHECK(parsed_and_validated_pre.warnings().empty());
		REQUIRE(parsed_and_validated_pre.errors().size() == 3);
		CHECK(parsed_and_validated_pre.errors()[0].get_id() == int_id);
		CHECK(parsed_and_validated_pre.errors()[1].get_id() == float_id);
		CHECK(parsed_and_validated_pre.errors()[2].get_id() == string_id);
	}
}

TEST_CASE_METHOD(int_var_fixture, "Variable IDs can only be copied", "[libenvpp]")
{
	constexpr auto prefix_name = "LIBENVPP_TESTING";

	auto pre = env::prefix(prefix_name);
	const auto int_id = pre.register_variable<int>("INT");
	using id_t = std::remove_cv_t<decltype(int_id)>;
	static_assert(!std::is_move_constructible_v<id_t>);
	static_assert(!std::is_move_assignable_v<id_t>);
	static_assert(std::is_copy_constructible_v<id_t>);
	static_assert(std::is_copy_assignable_v<id_t>);
	const auto int_id_copy_constructed = int_id;
	auto int_id_copy_assigned = int_id;
	int_id_copy_assigned = int_id_copy_constructed;
	auto parsed_and_validated_pre = pre.parse_and_validate();
	CHECK(parsed_and_validated_pre.ok());
	const auto int_id_val = parsed_and_validated_pre.get(int_id);
	const auto int_id_copy_constructed_val = parsed_and_validated_pre.get(int_id_copy_constructed);
	const auto int_id_copy_assigned_val = parsed_and_validated_pre.get(int_id_copy_assigned);
	REQUIRE(int_id_val.has_value());
	REQUIRE(int_id_copy_constructed_val.has_value());
	REQUIRE(int_id_copy_assigned_val.has_value());
	CHECK(*int_id_val == 42);
	CHECK(*int_id_copy_constructed_val == 42);
	CHECK(*int_id_copy_assigned_val == 42);
}

TEST_CASE("Empty prefix name throws", "[libenvpp]")
{
	CHECK_THROWS_AS(env::prefix(""), invalid_prefix);
}

TEST_CASE("Empty prefix can be validated", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	auto parsed_and_validated_pre = pre.parse_and_validate();
	CHECK(parsed_and_validated_pre.ok());
}

TEST_CASE("Prefix can be moved", "[libenvpp]")
{
	static_assert(!std::is_default_constructible_v<prefix>);
	static_assert(std::is_move_constructible_v<prefix>);
	static_assert(std::is_move_assignable_v<prefix>);
	static_assert(!std::is_copy_constructible_v<prefix>);
	static_assert(!std::is_copy_assignable_v<prefix>);

	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto int_var = pre.register_required_variable<int>("INT");
	auto moved_pre = std::move(pre);
	CHECK_THROWS_AS(pre.register_variable<float>("FLOAT"), invalidated_prefix);
	const auto float_var = moved_pre.register_required_variable<float>("FLOAT");
	moved_pre.set_for_testing(int_var, 7);
	pre = std::move(moved_pre);
	CHECK_THROWS_AS(moved_pre.register_variable<std::string>("STRING"), invalidated_prefix);
	pre.set_for_testing(float_var, 42.42f);
	auto parsed_and_validated_pre = pre.parse_and_validate();
	CHECK(parsed_and_validated_pre.ok());
	CHECK(parsed_and_validated_pre.get(int_var) == 7);
	CHECK(parsed_and_validated_pre.get(float_var) == 42.42f);
}

TEST_CASE("Invalidated prefix throws", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto var_id = pre.register_variable<int>("INT");
	pre.set_for_testing(var_id, 7);

	SECTION("Parsing prefix invalidates it") { auto parsed_and_validated_pre = pre.parse_and_validate(); }
	SECTION("Moving prefix invalidates it")
	{
		SECTION("Move construction") { auto moved_pre = std::move(pre); }
		SECTION("Move assignment")
		{
			auto moved_pre = std::move(pre);
			pre = std::move(moved_pre);
			moved_pre = std::move(pre);
		}
	}

	CHECK_THROWS_AS(pre.register_variable<int>("INT2"), invalidated_prefix);
	CHECK_THROWS_AS(pre.register_required_variable<int>("INT2"), invalidated_prefix);
	CHECK_THROWS_AS(pre.register_range<int>("INT2", 0, 1), invalidated_prefix);
	CHECK_THROWS_AS(pre.register_required_range<int>("INT2", 0, 1), invalidated_prefix);
	CHECK_THROWS_AS(pre.register_option<int>("INT2", {0}), invalidated_prefix);
	CHECK_THROWS_AS(pre.register_required_option<int>("INT2", {0}), invalidated_prefix);
	CHECK_THROWS_AS(pre.set_for_testing(var_id, 4), invalidated_prefix);
	CHECK_THROWS_AS(pre.parse_and_validate(), invalidated_prefix);
	CHECK_THROWS_AS(pre.help_message(), invalidated_prefix);
}

TEST_CASE("Parsed and validated prefix can be moved", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto int_var = pre.register_variable<int>("ENV_VAR");
	pre.set_for_testing(int_var, 7);
	auto parsed_and_validated_pre = pre.parse_and_validate();

	using parsed_and_validated_prefix_t = std::remove_cv_t<decltype(parsed_and_validated_pre)>;
	static_assert(!std::is_default_constructible_v<parsed_and_validated_prefix_t>);
	static_assert(std::is_move_constructible_v<parsed_and_validated_prefix_t>);
	static_assert(std::is_move_assignable_v<parsed_and_validated_prefix_t>);
	static_assert(!std::is_copy_constructible_v<parsed_and_validated_prefix_t>);
	static_assert(!std::is_copy_assignable_v<parsed_and_validated_prefix_t>);

	const auto check_prefix = [&int_var](const auto& parsed_pre) {
		CHECK(parsed_pre.ok());
		CHECK_THAT(parsed_pre.error_message(), IsEmpty());
		CHECK_THAT(parsed_pre.warning_message(), IsEmpty());
		CHECK(parsed_pre.errors().empty());
		CHECK(parsed_pre.warnings().empty());
		CHECK_THAT(parsed_pre.help_message(), ContainsSubstring("'LIBENVPP_TESTING_ENV_VAR' optional"));
		CHECK(parsed_pre.get_or(int_var, 4) == 7);
		CHECK(*parsed_pre.get(int_var) == 7);
	};

	SECTION("Move construction")
	{
		auto moved_parsed_pre = std::move(parsed_and_validated_pre);
		check_prefix(moved_parsed_pre);
	}
	SECTION("Move assignment")
	{
		auto moved_parsed_pre = std::move(parsed_and_validated_pre);
		parsed_and_validated_pre = std::move(moved_parsed_pre);
		moved_parsed_pre = std::move(parsed_and_validated_pre);
		check_prefix(moved_parsed_pre);
	}
}

TEST_CASE("Moved from parsed and validated prefix throws", "[libenvpp]")
{
	auto pre = env::prefix("LIBENVPP_TESTING");
	const auto int_var = pre.register_variable<int>("ENV_VAR");
	pre.set_for_testing(int_var, 7);
	auto parsed_and_validated_pre = pre.parse_and_validate();

	const auto check_prefix = [&int_var](const auto& parsed_pre) {
		CHECK_THROWS_AS(parsed_pre.ok(), invalidated_prefix);
		CHECK_THROWS_AS(parsed_pre.error_message(), invalidated_prefix);
		CHECK_THROWS_AS(parsed_pre.warning_message(), invalidated_prefix);
		CHECK_THROWS_AS(parsed_pre.errors().empty(), invalidated_prefix);
		CHECK_THROWS_AS(parsed_pre.warnings().empty(), invalidated_prefix);
		CHECK_THROWS_AS(parsed_pre.help_message(), invalidated_prefix);
		CHECK_THROWS_AS(parsed_pre.get_or(int_var, 4), invalidated_prefix);
		CHECK_THROWS_AS(parsed_pre.get(int_var), invalidated_prefix);
	};

	SECTION("Move construction")
	{
		auto moved_parsed_pre = std::move(parsed_and_validated_pre);
		check_prefix(parsed_and_validated_pre);
	}
	SECTION("Move assignment")
	{
		auto moved_parsed_pre = std::move(parsed_and_validated_pre);
		parsed_and_validated_pre = std::move(moved_parsed_pre);
		check_prefix(moved_parsed_pre);
	}
}

TEST_CASE_METHOD(int_var_fixture, "Retrieving integer with get", "[libenvpp][get]")
{
	const auto int_value = get<int>("LIBENVPP_TESTING_INT");
	REQUIRE(int_value.has_value());
	CHECK(*int_value == 42);
}

TEST_CASE_METHOD(float_var_fixture, "Retrieving float with get", "[libenvpp][get]")
{
	const auto float_value = get<float>("LIBENVPP_TESTING_FLOAT");
	REQUIRE(float_value.has_value());
	CHECK(*float_value == 3.1415f);
}

TEST_CASE_METHOD(string_var_fixture, "Retrieving string with get", "[libenvpp][get]")
{
	const auto string_value = get<std::string>("LIBENVPP_TESTING_STRING");
	REQUIRE(string_value.has_value());
	CHECK(*string_value == "Hello World");
}

TEST_CASE_METHOD(option_var_fixture, "Retrieving option with get", "[libenvpp][get]")
{
	const auto option_value = get<testing_option>("LIBENVPP_TESTING_OPTION");
	REQUIRE(option_value.has_value());
	CHECK(*option_value == testing_option::SECOND_OPTION);
}

TEST_CASE_METHOD(float_var_fixture, "Parsing error using get", "[libenvpp][get]")
{
	const auto int_value = get<int>("LIBENVPP_TESTING_FLOAT");
	CHECK_FALSE(int_value.has_value());
	CHECK_THAT(int_value.error().what(),
	           ContainsSubstring("Parser error") && ContainsSubstring("'LIBENVPP_TESTING_FLOAT'"));
}

TEST_CASE("Validation error using get", "[libenvpp][get]")
{
	const auto _ = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_UNVALIDATABLE", "FOO"};

	const auto value = get<unvalidatable_type>("LIBENVPP_TESTING_UNVALIDATABLE");
	CHECK_FALSE(value.has_value());
	CHECK_THAT(value.error().what(), ContainsSubstring("Validation error") && ContainsSubstring("Unvalidatable")
	                                     && ContainsSubstring("'LIBENVPP_TESTING_UNVALIDATABLE'"));
}

TEST_CASE("Environment variable does not exist when using get", "[libenvpp][get]")
{
	const auto value = get<int>("LIBENVPP_TESTING_INT");
	CHECK_FALSE(value.has_value());
	CHECK_THAT(value.error().what(), ContainsSubstring("'LIBENVPP_TESTING_INT' not set"));
}

TEST_CASE_METHOD(int_var_fixture, "Typo detection when using get", "[libenvpp][get]")
{
	const auto value = get<int>("LIBENVPP_TESTING_HINT");
	CHECK_FALSE(value.has_value());
	CHECK_THAT(value.error().what(), ContainsSubstring("'LIBENVPP_TESTING_INT' set")
	                                     && ContainsSubstring("did you mean 'LIBENVPP_TESTING_HINT'"));
}

TEST_CASE("Retrieving integer with get_or", "[libenvpp][get]")
{
	SECTION("Set environment variable")
	{
		const auto _ = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_ENV_VAR", "FOO"};
		const auto value = get_or<std::string>("LIBENVPP_TESTING_ENV_VAR", "BAR");
		CHECK_THAT(value, Equals("FOO"));
	}

	SECTION("Unset environment variable")
	{
		const auto value = get_or<std::string>("LIBENVPP_TESTING_ENV_VAR", "BAR");
		CHECK_THAT(value, Equals("BAR"));
	}
}

TEST_CASE("Errors yield default value with get_or", "[libenvpp][get]")
{
	SECTION("Parser error")
	{
		const auto _ = detail::set_scoped_environment_variable{"LIBENVPP_TESTING_ENV_VAR", "FOO"};
		const auto value = get_or<int>("LIBENVPP_TESTING_ENV_VAR", 7);
		CHECK(value == 7);
	}

	SECTION("Validation error")
	{
		CHECK_NOTHROW(get_or<unvalidatable_type>("LIBENVPP_TESTING_ENV_VAR", unvalidatable_type{"FOO"}));
	}
}

} // namespace env
