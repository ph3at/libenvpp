#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <fmt/core.h>

#include <libenvpp/detail/parser.hpp>

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

class string_constructible_6 {
  public:
	string_constructible_6(const std::string_view str) : m_str(std::string(str)) {}

	inline bool operator==(const string_constructible_6& other) const { return m_str == other.m_str; }

  private:
	std::string m_str;
};

static_assert(is_string_constructible_v<std::string> == true);
static_assert(is_string_constructible_v<std::string_view> == true);
static_assert(is_string_constructible_v<string_constructible_0> == false);
static_assert(is_string_constructible_v<string_constructible_1> == false);
static_assert(is_string_constructible_v<string_constructible_2> == true);
static_assert(is_string_constructible_v<string_constructible_3> == true);
static_assert(is_string_constructible_v<string_constructible_4> == true);
static_assert(is_string_constructible_v<string_constructible_5> == true);
static_assert(is_string_constructible_v<string_constructible_6> == true);
static_assert(is_string_constructible_v<char> == false);
static_assert(is_string_constructible_v<char*> == false);
static_assert(is_string_constructible_v<const char*> == false);
static_assert(is_string_constructible_v<void*> == false);
static_assert(is_string_constructible_v<const void*> == false);
static_assert(is_string_constructible_v<char&> == false);
static_assert(is_string_constructible_v<char*&> == false);
static_assert(is_string_constructible_v<bool> == false);
static_assert(is_string_constructible_v<int> == false);
static_assert(is_string_constructible_v<float> == false);

//////////////////////////////////////////////////////////////////////////

enum stream_constructible_0 {};
enum class stream_constructible_1 {};

enum stream_constructible_2 { STREAM_ENUM_VALUE_0, STREAM_ENUM_VALUE_1 };
enum class stream_constructible_3 { ENUM_CLASS_VALUE_0, ENUM_CLASS_VALUE_1 };

static std::istringstream& operator>>(std::istringstream& stream, stream_constructible_2& value)
{
	auto sentry = std::istringstream::sentry{stream};
	if (sentry) {
		auto str = std::string();
		stream >> str;
		if (str == "STREAM_ENUM_VALUE_0") {
			value = stream_constructible_2::STREAM_ENUM_VALUE_0;
		} else if (str == "STREAM_ENUM_VALUE_1") {
			value = stream_constructible_2::STREAM_ENUM_VALUE_1;
		} else {
			stream.setstate(std::ios_base::failbit);
		}
	}
	return stream;
}

static std::istringstream& operator>>(std::istringstream& stream, stream_constructible_3& value)
{
	auto sentry = std::istringstream::sentry{stream};
	if (sentry) {
		auto str = std::string();
		stream >> str;
		if (str == "ENUM_CLASS_VALUE_0") {
			value = stream_constructible_3::ENUM_CLASS_VALUE_0;
		} else if (str == "ENUM_CLASS_VALUE_1") {
			value = stream_constructible_3::ENUM_CLASS_VALUE_1;
		} else {
			stream.setstate(std::ios_base::failbit);
		}
	}
	return stream;
}

struct stream_constructible_4 {};

class stream_constructible_5 {
  public:
	stream_constructible_5() = default;
	stream_constructible_5(const int num) : m_num(num) {}
	inline bool operator==(const stream_constructible_5& other) const { return m_num == other.m_num; }

  private:
	int m_num;

	friend std::istringstream& operator>>(std::istringstream& stream, stream_constructible_5& value)
	{
		stream >> value.m_num;
		return stream;
	}
};

static_assert(is_stringstream_constructible_v<void> == false);
static_assert(is_stringstream_constructible_v<bool> == true);
static_assert(is_stringstream_constructible_v<char> == true);
static_assert(is_stringstream_constructible_v<int> == true);
static_assert(is_stringstream_constructible_v<float> == true);
static_assert(is_stringstream_constructible_v<void*> == true);
static_assert(is_stringstream_constructible_v<const void*> == false);
static_assert(is_stringstream_constructible_v<char*> == false);
static_assert(is_stringstream_constructible_v<const char*> == false);
static_assert(is_stringstream_constructible_v<int*> == false);
static_assert(is_stringstream_constructible_v<const int*> == false);
static_assert(is_stringstream_constructible_v<char&> == false);
static_assert(is_stringstream_constructible_v<const char&> == false);
static_assert(is_stringstream_constructible_v<char*&> == false);
static_assert(is_stringstream_constructible_v<int&> == false);
static_assert(is_stringstream_constructible_v<const int&> == false);
static_assert(is_stringstream_constructible_v<stream_constructible_0> == false);
static_assert(is_stringstream_constructible_v<stream_constructible_1> == false);
static_assert(is_stringstream_constructible_v<stream_constructible_2> == true);
static_assert(is_stringstream_constructible_v<stream_constructible_3> == true);
static_assert(is_stringstream_constructible_v<stream_constructible_4> == false);
static_assert(is_stringstream_constructible_v<stream_constructible_5> == true);

//////////////////////////////////////////////////////////////////////////

template <typename T>
static void test_parser(const std::string_view str, const T expected)
{
	CHECK_NOTHROW([&] {
		const auto parsed = construct_from_string<T>(str);
		CHECK((parsed == expected));
	}());
}

TEST_CASE("Parsing well-formed input of built-in type", "[libenvpp_parser]")
{
	SECTION("Primitive types")
	{
		test_parser<bool>("0", false);
		test_parser<bool>("00", false);
		test_parser<bool>("1", true);
		test_parser<bool>("false", false);
		test_parser<bool>("true", true);
		test_parser<bool>("False", false);
		test_parser<bool>("True", true);
		test_parser<bool>("FALSE", false);
		test_parser<bool>("TRUE", true);
		test_parser<bool>("off", false);
		test_parser<bool>("on", true);
		test_parser<bool>("Off", false);
		test_parser<bool>("On", true);
		test_parser<bool>("OFF", false);
		test_parser<bool>("ON", true);
		test_parser<bool>("no", false);
		test_parser<bool>("yes", true);
		test_parser<bool>("No", false);
		test_parser<bool>("Yes", true);
		test_parser<bool>("NO", false);
		test_parser<bool>("YES", true);

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
	}

	SECTION("Class types"){}
	{
		test_parser<std::string>("foo", "foo");
		test_parser<std::string>("BAR", "BAR");
	}

	SECTION("Leading and trailing whitespaces")
	{
		test_parser<bool>(" 1", true);
		test_parser<bool>("0 ", false);
		test_parser<bool>(" TrUe ", true);
		test_parser<bool>(" \r \t \n oFf \r \t \n ", false);
		test_parser<char>("a ", 'a');
		test_parser<short>(" -12345 ", -12345);
		test_parser<unsigned short>("\t65000", 65000);
		test_parser<int>("-123456789\t", -123456789);
		test_parser<unsigned int>("\t1234567890\t", 1234567890);
		test_parser<long>("\r-1234567890\n", -1234567890);
		test_parser<unsigned long>("\n3456789012\r", 3456789012);
		test_parser<long long>("\r\n-123456789012345678\r\n", -123456789012345678);
		test_parser<unsigned long long>(" \r \t \n 1234567890123456789 \r \t \n ", 1234567890123456789);
		test_parser<float>(" \t\r\n -.1234", -.1234f);
		test_parser<double>(" -.1234567890123456789 \t\r\n ", -.1234567890123456789);
	}
}

TEST_CASE("Parsing well-formed input of user-defined type", "[libenvpp_parser]")
{
	test_parser<string_constructible_6>("", string_constructible_6{""});
	test_parser<string_constructible_6>("foo", string_constructible_6{"foo"});
	test_parser<string_constructible_6>(" foo", string_constructible_6{" foo"});
	test_parser<string_constructible_6>("\tfoo ", string_constructible_6{"\tfoo "});

	test_parser<stream_constructible_2>("STREAM_ENUM_VALUE_0", stream_constructible_2::STREAM_ENUM_VALUE_0);
	test_parser<stream_constructible_2>(" STREAM_ENUM_VALUE_1", stream_constructible_2::STREAM_ENUM_VALUE_1);
	test_parser<stream_constructible_2>("STREAM_ENUM_VALUE_0 ", stream_constructible_2::STREAM_ENUM_VALUE_0);
	test_parser<stream_constructible_2>("\nSTREAM_ENUM_VALUE_1\t", stream_constructible_2::STREAM_ENUM_VALUE_1);

	test_parser<stream_constructible_3>("ENUM_CLASS_VALUE_0", stream_constructible_3::ENUM_CLASS_VALUE_0);
	test_parser<stream_constructible_3>(" ENUM_CLASS_VALUE_1", stream_constructible_3::ENUM_CLASS_VALUE_1);
	test_parser<stream_constructible_3>("ENUM_CLASS_VALUE_0 ", stream_constructible_3::ENUM_CLASS_VALUE_0);
	test_parser<stream_constructible_3>("\rENUM_CLASS_VALUE_1\n", stream_constructible_3::ENUM_CLASS_VALUE_1);

	test_parser<stream_constructible_5>("42", stream_constructible_5{42});
	test_parser<stream_constructible_5>(" 42", stream_constructible_5{42});
	test_parser<stream_constructible_5>("42 ", stream_constructible_5{42});
	test_parser<stream_constructible_5>(" 42 ", stream_constructible_5{42});
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
static void test_parser_error(const std::string_view str)
{
	CHECK_THROWS_AS(construct_from_string<T>(str), parser_error);
	CHECK_THROWS_WITH(construct_from_string<T>(str), ContainsSubstring(std::string(str)));
}

TEST_CASE("Parsing ill-formed input of primitive type", "[libenvpp_parser]")
{
	test_parser_error<bool>("");
	test_parser_error<bool>(" ");
	test_parser_error<bool>("a");
	test_parser_error<bool>("-1");
	test_parser_error<bool>("2");
	test_parser_error<bool>("yas");
	test_parser_error<bool>("nope");

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

struct not_string_constructible_2 {
	not_string_constructible_2(const std::string_view str)
	{
		throw parser_error{fmt::format("Failed to construct '{}'", str)};
	}
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

struct not_stream_constructible_3 {};
std::istringstream& operator>>(std::istringstream& stream, not_stream_constructible_3&)
{
	throw parser_error{fmt::format("Failed to construct '{}'", stream.str())};
}

TEST_CASE("Parsing ill-formed input of user-defined type", "[libenvpp_parser]")
{
	test_parser_error<stream_constructible_2>("");
	test_parser_error<stream_constructible_2>("asdf");
	test_parser_error<stream_constructible_2>("stream_enum_value_0");
	test_parser_error<stream_constructible_2>("STREAM_ENUM_VALUE_2");

	test_parser_error<stream_constructible_3>("");
	test_parser_error<stream_constructible_3>("asdf");
	test_parser_error<stream_constructible_3>("enum_class_value_0");
	test_parser_error<stream_constructible_3>("ENUM_CLASS_VALUE_2");

	test_parser_error<stream_constructible_5>("");
	test_parser_error<stream_constructible_5>("asdf");
	test_parser_error<stream_constructible_5>("12345678901");
	test_parser_error<stream_constructible_5>("-12345678901");

	test_parser_error<not_string_constructible_0>("not_string_constructible_0");
	test_parser_error<not_string_constructible_1>("not_string_constructible_1");
	test_parser_error<not_string_constructible_2>("not_string_constructible_2");

	test_parser_error<not_stream_constructible_0>("not_stream_constructible_0");
	test_parser_error<not_stream_constructible_1>("not_stream_constructible_1");
	test_parser_error<not_stream_constructible_2>("not_stream_constructible_2");
	test_parser_error<not_stream_constructible_3>("not_stream_constructible_3");
}

} // namespace env::detail
