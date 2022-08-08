#include <iostream>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include <libenvpp.hpp>

TEST_CASE("Constructors and move semantics", "[libenvpp]")
{
	auto pre = env::prefix("FOO");
	auto var = pre.register_variable<int>("BAR");

	auto moved_var = std::move(var);
	moved_var.set_range(0, 16);
	var = std::move(moved_var);

	auto moved_pre = std::move(pre);
	pre = std::move(moved_pre);

	auto opt = pre.register_option<std::string>("BAZ").options({"ASDF", "QWERT"}).required();
	auto moved_opt = std::move(opt);
	opt = std::move(moved_opt);
}

TEST_CASE("Interface", "[libenvpp]")
{
	auto pre = env::prefix("PREFIX");
	auto name = pre.register_variable<std::string>("NAME");
	auto nodes = pre.register_variable<int>("NODES").range(1, 16);
	nodes.set_range(0, 5);

	enum class Platform { Windows, Linux, Mac };
	auto platform =
	    pre.register_option<Platform>("PLATFORM").options({Platform::Windows, Platform::Linux, Platform::Mac});
	platform.set_options({Platform::Linux});

	auto names = pre.register_option<std::string>("NAMES").options({"Steve", "Alex"});

	auto res = pre.validate();

	if (!res.ok()) {
		std::cout << res.error_message() << std::endl;
		std::cout << res.warning_message() << std::endl;

		for (const auto& error : res.errors()) {
			std::cout << error.what() << std::endl;
		}

		for (const auto& warning : res.warnings()) {
			std::cout << warning.what() << std::endl;
		}
	}
}
