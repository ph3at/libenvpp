#include <catch2/catch_test_macros.hpp>

#include <libenvpp/detail/check.hpp>
#include <libenvpp/detail/levenshtein.hpp>

TEST_CASE("Empty input", "[levenshtein]")
{
	CHECK(levenshtein::distance("", "") == 0);
	CHECK(levenshtein::distance("a", "") == 1);
	CHECK(levenshtein::distance("", "b") == 1);
	CHECK(levenshtein::distance("Hello World", "") == 11);
	CHECK(levenshtein::distance("", "Foo Bar Baz") == 11);
}

TEST_CASE("Identical inputs", "[levenshtein]")
{
	CHECK(levenshtein::distance("a", "a") == 0);
	CHECK(levenshtein::distance("B", "B") == 0);
	CHECK(levenshtein::distance("?", "?") == 0);
	CHECK(levenshtein::distance("Hello World!", "Hello World!") == 0);
}

TEST_CASE("Different lengths", "[levenshtein]")
{
	CHECK(levenshtein::distance("asdf", "asd") == 1);
	CHECK(levenshtein::distance("asd", "asdf") == 1);
	CHECK(levenshtein::distance("Hello World", "Hello") == 6);
	CHECK(levenshtein::distance("World", "Hello World") == 6);
}

TEST_CASE("Missing substrings", "[levenshtein]")
{
	CHECK(levenshtein::distance("abcdefg", "defg") == 3);
	CHECK(levenshtein::distance("abcdefg", "abc") == 4);
	CHECK(levenshtein::distance("abcdefg", "ag") == 5);
}

TEST_CASE("Different strings", "[levenshtein]")
{
	CHECK(levenshtein::distance("?", "!") == 1);
	CHECK(levenshtein::distance("asdf", "qwer") == 4);
	CHECK(levenshtein::distance("abcde", "vwxyz") == 5);
}

TEST_CASE("Reversed strings", "[levenshtein]")
{
	CHECK(levenshtein::distance("asdf", "fdsa") == 4);
	CHECK(levenshtein::distance("QWERT", "TREWQ") == 4);
	CHECK(levenshtein::distance("foo", "oof") == 2);
}

TEST_CASE("Cutoff distance", "[levenshtein]")
{
	CHECK_THROWS_AS(levenshtein::distance("a", "b", -1), env::detail::check_failed);
	CHECK(levenshtein::distance("a", "b", 0) == 0);
	CHECK(levenshtein::distance("Hello World", "Hello World", 7) == 0);
	CHECK(levenshtein::distance("Hello World", "HelloWorld", 1) == 1);
	CHECK(levenshtein::distance("Hello World", "HelloWorld", 2) == 1);
	CHECK(levenshtein::distance("Hello World", "World", 3) == 3);
}

TEST_CASE("Less than wrapper", "[levenshtein]")
{
	CHECK_THROWS_AS(levenshtein::is_distance_less_than("a", "b", -5), env::detail::check_failed);
	CHECK(levenshtein::is_distance_less_than("", "", 1));
	CHECK(levenshtein::is_distance_less_than("a", "a", 1));
	CHECK_FALSE(levenshtein::is_distance_less_than("a", "b", 1));
	CHECK(levenshtein::is_distance_less_than("a", "b", 2));
	CHECK_FALSE(levenshtein::is_distance_less_than("Hello World", "Hello World", 0));
	CHECK(levenshtein::is_distance_less_than("Hello World", "Hello World", 1));
	CHECK_FALSE(levenshtein::is_distance_less_than("Hello World", "HloWrd", 5));
	CHECK(levenshtein::is_distance_less_than("Hello World", "HloWrd", 6));
}
