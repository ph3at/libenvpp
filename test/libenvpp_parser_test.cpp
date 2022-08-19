#include <charconv>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <libenvpp_parser.hpp>
#include <libenvpp_util.hpp>

namespace env::detail {

using Catch::Matchers::ContainsSubstring;

struct string_constructible_0 {};

struct string_constructible_1 {
	string_constructible_1(const char*) {}
};

struct string_constructible_2 {
	string_constructible_2(std::string) {}
};

struct string_constructible_3 {
	string_constructible_3(std::string_view) {}
};

struct string_constructible_4 {
	explicit string_constructible_4(std::string) {}
};

struct string_constructible_5 {
	explicit string_constructible_5(std::string_view) {}
};

static_assert(is_string_constructible_v<std::string> == true);
static_assert(is_string_constructible_v<std::string_view> == true);
static_assert(is_string_constructible_v<string_constructible_0> == false);
static_assert(is_string_constructible_v<string_constructible_1> == false);
static_assert(is_string_constructible_v<string_constructible_2> == true);
static_assert(is_string_constructible_v<string_constructible_3> == true);
static_assert(is_string_constructible_v<string_constructible_4> == true);
static_assert(is_string_constructible_v<string_constructible_5> == true);
static_assert(is_string_constructible_v<char> == false);
static_assert(is_string_constructible_v<char*> == false);
static_assert(is_string_constructible_v<bool> == false);
static_assert(is_string_constructible_v<int> == false);
static_assert(is_string_constructible_v<float> == false);

//////////////////////////////////////////////////////////////////////////

enum stream_constructible_0 {};
enum class stream_constructible_1 {};

enum stream_constructible_2 { STREAM_ENUM_VALUE };
enum class stream_constructible_3 { ENUM_CLASS_VALUE };

std::istringstream& operator>>(std::istringstream& stream, stream_constructible_2& value)
{
	if (stream.str() == "STREAM_ENUM_VALUE") {
		value = stream_constructible_2::STREAM_ENUM_VALUE;
	} else {
		stream.setstate(std::ios_base::failbit);
	}
	return stream;
}

std::istringstream& operator>>(std::istringstream& stream, stream_constructible_3& value)
{
	if (stream.str() == "ENUM_CLASS_VALUE") {
		value = stream_constructible_3::ENUM_CLASS_VALUE;
	} else {
		stream.setstate(std::ios_base::failbit);
	}
	return stream;
}

struct stream_constructible_4 {};

struct stream_constructible_5 {
	int a;
};

std::istream& operator>>(std::istream& stream, stream_constructible_5& value)
{
	return stream >> value.a;
}

static_assert(is_stringstream_constructible_v<void> == false);
static_assert(is_stringstream_constructible_v<bool> == true);
static_assert(is_stringstream_constructible_v<char> == true);
static_assert(is_stringstream_constructible_v<char*> == true);
static_assert(is_stringstream_constructible_v<int> == true);
static_assert(is_stringstream_constructible_v<float> == true);
static_assert(is_stringstream_constructible_v<stream_constructible_0> == false);
static_assert(is_stringstream_constructible_v<stream_constructible_1> == false);
static_assert(is_stringstream_constructible_v<stream_constructible_2> == true);
static_assert(is_stringstream_constructible_v<stream_constructible_3> == true);
static_assert(is_stringstream_constructible_v<stream_constructible_4> == false);
static_assert(is_stringstream_constructible_v<stream_constructible_5> == true);

TEST_CASE("Parsing using stream operator>>", "[libenvpp_parser]")
{
	auto stream = std::istringstream{};

	SECTION("enum")
	{
		auto enum_value = stream_constructible_2{};

		stream.str("STREAM_ENUM_VALUE");
		stream >> enum_value;
		REQUIRE_FALSE(stream.fail());
		CHECK(enum_value == stream_constructible_2::STREAM_ENUM_VALUE);

		stream.str("asdf");
		stream >> enum_value;
		CHECK(stream.fail());
	}

	SECTION("enum class")
	{
		auto enum_class_value = stream_constructible_3{};

		stream.str("ENUM_CLASS_VALUE");
		stream >> enum_class_value;
		REQUIRE_FALSE(stream.fail());
		CHECK(enum_class_value == stream_constructible_3::ENUM_CLASS_VALUE);

		stream.str("asdf");
		stream >> enum_class_value;
		CHECK(stream.fail());
	}

	SECTION("struct")
	{
		auto struct_value = stream_constructible_5{};

		stream.str("42");
		stream >> struct_value;
		REQUIRE_FALSE(stream.fail());
		CHECK(struct_value.a == 42);

		stream.str("asdf");
		stream >> struct_value;
		CHECK(stream.fail());
	}
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
void test_parser(const std::string_view str, const T expected)
{
	CHECK_NOTHROW([&] {
		const auto parsed = construct_from_string<T>(str);
		CHECK(parsed == expected);
	}());
}

TEST_CASE("Parsing well-formed input of primitive type", "[libenvpp_parser]")
{
	test_parser<bool>("0", false);
	test_parser<bool>("1", true);

	test_parser<char>("0", '0');
	test_parser<char>("a", 'a');
	test_parser<char>("A", 'A');

	test_parser<short>("-12345", -12345);
	test_parser<short>("0", 0);
	test_parser<short>("12345", 12345);
	test_parser<unsigned short>("0", 0);
	test_parser<unsigned short>("65000", 65000);

	test_parser<int>("-123456789", -123456789);
	test_parser<int>("0", 0);
	test_parser<int>("123456789", 123456789);
	test_parser<unsigned int>("1234567890", 1234567890);
	test_parser<unsigned int>("0", 0);

	test_parser<long>("-1234567890", -1234567890);
	test_parser<long>("0", 0);
	test_parser<long>("1234567890", 1234567890);
	test_parser<unsigned long>("0", 0);
	test_parser<unsigned long>("3456789012", 3456789012);

	test_parser<long long>("-123456789012345678", -123456789012345678);
	test_parser<long long>("0", 0);
	test_parser<long long>("123456789012345678", 123456789012345678);
	test_parser<unsigned long long>("0", 0);
	test_parser<unsigned long long>("1234567890123456789", 1234567890123456789);

	test_parser<float>("-1", -1.f);
	test_parser<float>("0", 0.f);
	test_parser<float>("1", 1.f);
	test_parser<float>("3.1415", 3.1415f);
	test_parser<float>("0.1", 0.1f);
	test_parser<float>(".2", .2f);
	test_parser<float>("0.123", 0.123f);
	test_parser<float>("0.33333", 0.33333f);
	test_parser<float>("123456789", 123456789.f);
	test_parser<float>("-123456789", -123456789.f);
	test_parser<float>("123456789.1", 123456789.1f);

	test_parser<double>("-1", -1.0);
	test_parser<double>("0", 0);
	test_parser<double>("1", 1.0);
	test_parser<double>("3.1415926535", 3.1415926535);
	test_parser<double>("1234567890123456789", 1234567890123456789.0);
	test_parser<double>("-1234567890123456789", -1234567890123456789.0);
	test_parser<double>("0.1234567890123456789", 0.1234567890123456789);
	test_parser<double>("-0.1234567890123456789", -0.1234567890123456789);
	test_parser<double>("1234567890.0123456789", 1234567890.0123456789);
	test_parser<double>("-1234567890.0123456789", -1234567890.0123456789);

	test_parser<std::string>("", "");
	test_parser<std::string>("foo", "foo");
	test_parser<std::string>("BAR", "BAR");
}

class string_constructible {
  public:
	string_constructible(const std::string_view str) : m_str(str) {}

	bool operator==(const string_constructible& other) const { return m_str == other.m_str; }

  private:
	const std::string m_str;
};

struct stream_constructible {
	bool operator==(const stream_constructible& other) const { return m_str == other.m_str; }

	std::string m_str;
};
std::istringstream& operator>>(std::istringstream& stream, stream_constructible& value)
{
	stream >> value.m_str;
	return stream;
}

TEST_CASE("Parsing well-formed input of user-defined type", "[libenvpp_parser]")
{
	test_parser<string_constructible>("foo", string_constructible{"foo"});
	test_parser<stream_constructible>("baz", stream_constructible{"baz"});
}

template <typename T>
void test_parser_error(const std::string_view str)
{
	CHECK_THROWS_AS(construct_from_string<T>(str), parser_error);
	CHECK_THROWS_WITH(construct_from_string<T>(str), ContainsSubstring(std::string(str)));
}

TEST_CASE("Parsing ill-formed input of primitive type", "[libenvpp_parser]")
{
	test_parser_error<bool>("");
	test_parser_error<bool>(" ");
	test_parser_error<bool>("a");
	test_parser_error<bool>("true");
	test_parser_error<bool>("false");

	test_parser_error<short>("");
	test_parser_error<short>(" ");
	test_parser_error<short>("a");
	test_parser_error<short>("z");
	test_parser_error<short>("-123456");
	test_parser_error<short>("123456");
	test_parser_error<unsigned short>("123456");

	test_parser_error<int>("-12345678901");
	test_parser_error<int>("12345678901");
	test_parser_error<unsigned int>("12345678901");

	test_parser_error<long>("-123456789012345678901");
	test_parser_error<long>("123456789012345678901");
	test_parser_error<unsigned long>("123456789012345678901");

	test_parser_error<long long>("-123456789012345678901");
	test_parser_error<long long>("123456789012345678901");
	test_parser_error<unsigned long long>("123456789012345678901");

	test_parser_error<float>("a");

	test_parser_error<double>("b");
}

struct not_string_constructible_0 {
	not_string_constructible_0(const std::string_view) { throw 0; }
};

struct not_string_constructible_1 {
	not_string_constructible_1(const std::string_view) { throw std::runtime_error{"Unconstructible"}; }
};

struct not_stream_constructible_0 {};
std::istringstream& operator>>(std::istringstream& stream, not_stream_constructible_0&)
{
	stream.setstate(std::ios_base::failbit);
	return stream;
}

struct not_stream_constructible_1 {};
std::istringstream& operator>>(std::istringstream&, not_stream_constructible_1&)
{
	throw 0;
}

struct not_stream_constructible_2 {};
std::istringstream& operator>>(std::istringstream&, not_stream_constructible_2&)
{
	throw std::runtime_error{"Unconstructible"};
}

TEST_CASE("Parsing ill-formed input of user-defined type", "[libenvpp_parser]")
{
	test_parser_error<not_string_constructible_0>("don't care");
	test_parser_error<not_string_constructible_1>("don't care");

	test_parser_error<not_stream_constructible_0>("don't care");
	test_parser_error<not_stream_constructible_1>("don't care");
	test_parser_error<not_stream_constructible_2>("don't care");
}

} // namespace env::detail
