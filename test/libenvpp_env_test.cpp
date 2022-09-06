#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <libenvpp_env.hpp>

namespace env::detail {

using Catch::Matchers::Equals;
using Catch::Matchers::StartsWith;

#if LIBENVPP_PLATFORM_WINDOWS

TEST_CASE("Converting wide-char to multi-byte", "[libenvpp_env]")
{
	SECTION("Valid input")
	{
		constexpr auto input = L"foo 🍌 bar";
		constexpr auto output = "foo 🍌 bar";

		const auto wide_char_str = std::wstring(input);
		const auto multi_byte_str = convert_string(wide_char_str);
		REQUIRE(multi_byte_str.has_value());
		CHECK_THAT(*multi_byte_str, Equals(output));
	}

	SECTION("Invalid input parsed at least up to invalid character")
	{
		constexpr auto input = L"foo \xd800 bar";

		const auto wide_char_str = std::wstring(input);
		const auto multi_byte_str = convert_string(wide_char_str);
		REQUIRE(multi_byte_str.has_value());
		CHECK_THAT(*multi_byte_str, StartsWith("foo "));
	}

	SECTION("Invalid input replaced with unicode replacement character")
	{
		constexpr auto input = L"foo \xd800 bar";
		constexpr auto output = "foo � bar";

		const auto wide_char_str = std::wstring(input);
		const auto multi_byte_str = convert_string(wide_char_str);
		REQUIRE(multi_byte_str.has_value());
		CHECK_THAT(*multi_byte_str, Equals(output));
	}
}

TEST_CASE("Converting multi-byte to wide-char", "[libenvpp_env]")
{
	SECTION("Valid input")
	{
		constexpr auto input = "foo 🍌 bar";
		constexpr auto output = L"foo 🍌 bar";

		const auto multi_byte_str = std::string(input);
		const auto wide_char_str = convert_string(multi_byte_str);
		REQUIRE(wide_char_str.has_value());
		CHECK(*wide_char_str == output); // Catch2 does not have matcher support for wide-chars
	}

	SECTION("Invalid input parsed at least up to invalid character")
	{
		constexpr auto input = "foo \x80 bar";

		const auto multi_byte_str = std::string(input);
		const auto wide_char_str = convert_string(multi_byte_str);
		REQUIRE(wide_char_str.has_value());
		CHECK(wide_char_str->find(L"foo ") == 0); // Catch2 does not have matcher support for wide-chars
	}

	SECTION("Invalid input parsed at least up to invalid character")
	{
		constexpr auto input = "foo \x80 bar";

		const auto multi_byte_str = std::string(input);
		const auto wide_char_str = convert_string(multi_byte_str);
		REQUIRE(wide_char_str.has_value());
		CHECK(wide_char_str->find(L"foo ") == 0); // Catch2 does not have matcher support for wide-chars
	}

	SECTION("Invalid input replaced with unicode replacement character")
	{
		constexpr auto input = "foo \x80 bar";
		constexpr auto output = L"foo � bar";

		const auto multi_byte_str = std::string(input);
		const auto wide_char_str = convert_string(multi_byte_str);
		REQUIRE(wide_char_str.has_value());
		CHECK(*wide_char_str == output); // Catch2 does not have matcher support for wide-chars
	}
}

#endif

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

TEST_CASE("Character encoding for variable names", "[libenvpp_env]")
{
	SECTION("Valid input")
	{
		constexpr auto test_var_name = "LIBENVPP_TESTING_🍌";
		constexpr auto test_var_value = "banana";

		const auto _ = set_scoped_environment_variable{test_var_name, test_var_value};

		const auto test_var = get_environment_variable(test_var_name);
		REQUIRE(test_var.has_value());
		CHECK_THAT(*test_var, Equals(test_var_value));

		const auto environment = get_environment();
		REQUIRE_FALSE(environment.empty());

		REQUIRE(environment.find(test_var_name) != environment.end());
		CHECK_THAT(environment.at(test_var_name), Equals(test_var_value));
	}

	SECTION("Invalid input")
	{
		constexpr auto test_var_name = "LIBENVPP_TESTING_\x80";
		constexpr auto test_var_value = "invalid utf-8";

		const auto _ = set_scoped_environment_variable{test_var_name, test_var_value};

		const auto test_var = get_environment_variable(test_var_name);
		REQUIRE(test_var.has_value());
		CHECK_THAT(*test_var, Equals(test_var_value));

		const auto environment = get_environment();
		REQUIRE_FALSE(environment.empty());

		constexpr auto var_name = LIBENVPP_PLATFORM_WINDOWS ? "LIBENVPP_TESTING_�" : test_var_name;

		REQUIRE(environment.find(var_name) != environment.end());
		CHECK_THAT(environment.at(var_name), Equals(test_var_value));
	}
}

TEST_CASE("Character encoding for variable values", "[libenvpp_env]")
{
	SECTION("Valid input")
	{
		constexpr auto test_var_name = "LIBENVPP_TESTING_BANANA";
		constexpr auto test_var_value = "->🍌<-";

		const auto _ = set_scoped_environment_variable{test_var_name, test_var_value};

		const auto test_var = get_environment_variable(test_var_name);
		REQUIRE(test_var.has_value());
		CHECK_THAT(*test_var, Equals(test_var_value));

		const auto environment = get_environment();
		REQUIRE_FALSE(environment.empty());

		REQUIRE(environment.find(test_var_name) != environment.end());
		CHECK_THAT(environment.at(test_var_name), Equals(test_var_value));
	}

	SECTION("Invalid input")
	{
		constexpr auto test_var_name = "LIBENVPP_TESTING_INVALID_UTF-8";
		constexpr auto test_var_value = "->\x80<-";
		constexpr auto var_value = LIBENVPP_PLATFORM_WINDOWS ? "->�<-" : test_var_value;

		const auto _ = set_scoped_environment_variable{test_var_name, test_var_value};

		const auto test_var = get_environment_variable(test_var_name);
		REQUIRE(test_var.has_value());
		CHECK_THAT(*test_var, Equals(var_value));

		const auto environment = get_environment();
		REQUIRE_FALSE(environment.empty());

		REQUIRE(environment.find(test_var_name) != environment.end());
		CHECK_THAT(environment.at(test_var_name), Equals(var_value));
	}
}

} // namespace env::detail
