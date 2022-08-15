#include <catch2/catch_test_macros.hpp>

#include <charconv>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include <libenvpp_parser.hpp>

namespace adl::test {

struct from_string_able {};
void from_string(const std::string_view, std::optional<from_string_able>&);
static_assert(env::detail::has_from_string_fn_v<from_string_able> == true);

} // namespace adl::test

namespace env::detail {

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

struct from_string_mem_able_0 {};

struct from_string_mem_able_1 {
	static from_string_mem_able_1 from_string(std::string);
};

struct from_string_mem_able_2 {
	static from_string_mem_able_2 from_string(std::string_view);
};

struct from_string_mem_able_3 {
	static from_string_mem_able_3 from_string(char*);
};

struct from_string_mem_able_4 {
	static from_string_mem_able_4 from_string(int);
};

struct from_string_mem_able_5 {
	static from_string_mem_able_5 from_string(const std::string&);
};

struct from_string_mem_able_6 {
	static from_string_mem_able_6 from_string(std::string&);
};

struct from_string_mem_able_7 {
	static from_string_mem_able_7 from_string(const std::string_view&);
};

struct from_string_mem_able_8 {
	static from_string_mem_able_8 from_string(std::string_view&);
};

static_assert(has_from_string_mem_fn_v<from_string_mem_able_0> == false);
static_assert(has_from_string_mem_fn_v<from_string_mem_able_1> == true);
static_assert(has_from_string_mem_fn_v<from_string_mem_able_2> == true);
static_assert(has_from_string_mem_fn_v<from_string_mem_able_3> == false);
static_assert(has_from_string_mem_fn_v<from_string_mem_able_4> == false);
static_assert(has_from_string_mem_fn_v<from_string_mem_able_5> == true);
static_assert(has_from_string_mem_fn_v<from_string_mem_able_6> == false);
static_assert(has_from_string_mem_fn_v<from_string_mem_able_7> == true);
static_assert(has_from_string_mem_fn_v<from_string_mem_able_8> == false);
static_assert(has_from_string_mem_fn_v<int> == false);

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

static_assert(has_from_string_fn_v<from_string_able_0> == false);
static_assert(has_from_string_fn_v<from_string_able_1> == false);
static_assert(has_from_string_fn_v<from_string_able_2> == true);
static_assert(has_from_string_fn_v<from_string_able_3> == true);
static_assert(has_from_string_fn_v<from_string_able_4> == false);
static_assert(has_from_string_fn_v<from_string_able_5> == true);
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

} // namespace env::detail
