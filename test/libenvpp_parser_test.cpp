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

namespace adl::test {

struct from_string_able {};
void from_string(const std::string_view, std::optional<from_string_able>&);
static_assert(env::detail::has_from_string_fn_v<from_string_able> == true);

} // namespace adl::test

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

enum from_string_able_0 {};
enum class from_string_able_1 {};

enum from_string_able_2 { FROM_STRING_ENUM_VALUE };
enum class from_string_able_3 { ENUM_CLASS_VALUE };

void from_string(const std::string_view str, std::optional<from_string_able_2>& value)
{
	if (str == "FROM_STRING_ENUM_VALUE") {
		value = from_string_able_2::FROM_STRING_ENUM_VALUE;
	} else {
		value = std::nullopt;
	}
}

void from_string(const std::string_view str, std::optional<from_string_able_3>& value)
{
	if (str == "ENUM_CLASS_VALUE") {
		value = from_string_able_3::ENUM_CLASS_VALUE;
	} else {
		value = std::nullopt;
	}
}

struct from_string_able_4 {};

struct from_string_able_5 {
	int a;
};

void from_string(const std::string_view str, std::optional<from_string_able_5>& value)
{
	if (int parsed; std::from_chars(str.data(), str.data() + str.size(), parsed).ec == std::errc{}) {
		value = from_string_able_5{parsed};
	} else {
		value = std::nullopt;
	}
}

struct from_string_able_6 {};
void from_string(const std::string_view, std::optional<from_string_able_6>);

struct from_string_able_7 {};
void from_string(std::string_view, std::optional<from_string_able_7>&);

struct from_string_able_8 {};
void from_string(const std::string_view&, std::optional<from_string_able_8>&);

struct from_string_able_9 {};
void from_string(std::string_view&, std::optional<from_string_able_9>&);

struct from_string_able_10 {};
void from_string(std::string, std::optional<from_string_able_10>&);

struct from_string_able_11 {};
void from_string(const std::string, std::optional<from_string_able_11>&);

struct from_string_able_12 {};
void from_string(std::string&, std::optional<from_string_able_12>&);

struct from_string_able_13 {};
void from_string(const std::string&, std::optional<from_string_able_13>&);

struct from_string_able_14 {};
void from_string(const std::string_view, const std::optional<from_string_able_14>&);

struct from_string_able_15 {};
void from_string(const std::string_view, std::optional<from_string_able_15>&&);

struct from_string_able_16 {};
std::string from_string(const std::string_view, std::optional<from_string_able_16>&);

static_assert(has_from_string_fn_v<from_string_able_0> == false);
static_assert(has_from_string_fn_v<from_string_able_1> == false);
static_assert(has_from_string_fn_v<from_string_able_2> == true);
static_assert(has_from_string_fn_v<from_string_able_3> == true);
static_assert(has_from_string_fn_v<from_string_able_4> == false);
static_assert(has_from_string_fn_v<from_string_able_5> == true);
static_assert(has_from_string_fn_v<from_string_able_6> == false);
static_assert(has_from_string_fn_v<from_string_able_7> == true);
static_assert(has_from_string_fn_v<from_string_able_8> == true);
static_assert(has_from_string_fn_v<from_string_able_9> == false);
static_assert(has_from_string_fn_v<from_string_able_10> == true);
static_assert(has_from_string_fn_v<from_string_able_11> == true);
static_assert(has_from_string_fn_v<from_string_able_12> == false);
static_assert(has_from_string_fn_v<from_string_able_13> == true);
static_assert(has_from_string_fn_v<from_string_able_14> == false);
static_assert(has_from_string_fn_v<from_string_able_15> == false);
static_assert(has_from_string_fn_v<from_string_able_16> == true);
static_assert(has_from_string_fn_v<int> == false);

TEST_CASE("Parsing using free standing from_string function", "[libenvpp_parser]")
{
	SECTION("enum")
	{
		auto enum_value = std::optional<from_string_able_2>{};

		from_string("FROM_STRING_ENUM_VALUE", enum_value);
		REQUIRE(enum_value.has_value());
		CHECK(*enum_value == from_string_able_2::FROM_STRING_ENUM_VALUE);

		from_string("asdf", enum_value);
		CHECK_FALSE(enum_value.has_value());
	}

	SECTION("enum class")
	{
		auto enum_class_value = std::optional<from_string_able_3>{};

		from_string("ENUM_CLASS_VALUE", enum_class_value);
		REQUIRE(enum_class_value.has_value());
		CHECK(*enum_class_value == from_string_able_3::ENUM_CLASS_VALUE);

		from_string("asdf", enum_class_value);
		CHECK_FALSE(enum_class_value.has_value());
	}

	SECTION("struct")
	{
		auto struct_value = std::optional<from_string_able_5>{};

		from_string("42", struct_value);
		REQUIRE(struct_value.has_value());
		CHECK(struct_value->a == 42);

		from_string("qwer", struct_value);
		CHECK_FALSE(struct_value.has_value());
	}
}

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
	auto value = std::optional<T>{};
	const auto res = construct_from_string(str, value);
	CHECK(res.empty());
	REQUIRE(value.has_value());
	CHECK(*value == expected);
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

struct from_string_able {
	bool operator==(const from_string_able& other) const { return m_str == other.m_str; }

	std::string m_str;
};
void from_string(const std::string_view str, std::optional<from_string_able>& value)
{
	value = from_string_able{std::string(str)};
}

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
	test_parser<from_string_able>("bar", from_string_able{"bar"});
	test_parser<stream_constructible>("baz", stream_constructible{"baz"});
}

template <typename T>
void test_parser_error(const std::string_view str)
{
	auto value = std::optional<T>{};
	const auto err = construct_from_string(str, value);
	CHECK_FALSE(value.has_value());
	CHECK_FALSE(err.empty());
	CHECK_THAT(err, ContainsSubstring(std::string(str)));
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

struct not_from_string_able_0 {};
void from_string(const std::string_view, std::optional<not_from_string_able_0>&) {}

struct not_from_string_able_1 {};
void from_string(const std::string_view, std::optional<not_from_string_able_1>&)
{
	throw 0;
}

struct not_from_string_able_2 {};
void from_string(const std::string_view, std::optional<not_from_string_able_2>&)
{
	throw std::runtime_error{"Unconstructible"};
}

struct not_from_string_able_3 {};
std::string from_string(const std::string_view str, std::optional<not_from_string_able_3>&)
{
	return fmt::format("Not constructible from '{}'", str);
}

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

	test_parser_error<not_from_string_able_0>("don't care");
	test_parser_error<not_from_string_able_1>("don't care");
	test_parser_error<not_from_string_able_2>("don't care");
	test_parser_error<not_from_string_able_3>("don't care");

	test_parser_error<not_stream_constructible_0>("don't care");
	test_parser_error<not_stream_constructible_1>("don't care");
	test_parser_error<not_stream_constructible_2>("don't care");
}

} // namespace env::detail
