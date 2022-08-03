#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <fmt/core.h>

#include <libenvpp.hpp>

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::EndsWith;

TEST_CASE("Hello World", "[setup]")
{
	const auto hello_world = fmt::format("{} {}", env::HELLO_WORLD, env::test());
	CHECK_THAT(hello_world, ContainsSubstring(env::HELLO_WORLD) && EndsWith("7"));
}
