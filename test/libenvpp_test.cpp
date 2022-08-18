#include <iostream>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <libenvpp.hpp>

namespace env {

using Catch::Matchers::Equals;

TEST_CASE("Interface", "[libenvpp]")
{
	auto pre = env::prefix("PREFIX");
	const auto name_id = pre.register_variable<std::string>("NAME");
	[[maybe_unused]] const auto number_id =
	    pre.register_variable<int>("NUMBER", env::default_parser{[](const int& val) {
		                               if (val != 42) {
			                               throw std::runtime_error{"Wrong!"};
		                               }
	                               }});
	[[maybe_unused]] const auto string_id =
	    pre.register_variable<std::string>("STRING", [](const std::string_view str) {
		    if (str != "42") {
			    throw std::runtime_error{"Wrong!"};
		    }
		    return std::string(str);
	    });

	auto validated_pre = pre.validate();

	if (!validated_pre.ok()) {
		std::cout << validated_pre.error_message() << std::endl;
		std::cout << validated_pre.warning_message() << std::endl;

		for (const auto& error : validated_pre.errors()) {
			std::cout << error.what() << std::endl;
		}

		for (const auto& warning : validated_pre.warnings()) {
			std::cout << warning.what() << std::endl;
		}
		return;
	}

	const auto name = validated_pre.get(name_id);
	CHECK_THAT(*name, Equals("7TODO"));
}

} // namespace env
