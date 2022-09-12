#include <cstdlib>
#include <iostream>
#include <string>

#include <libenvpp.hpp>

int main()
{
	auto pre = env::prefix("SIMPLE_USAGE");

	const auto name_id = pre.register_variable<std::string>("NAME");
	const auto number_id = pre.register_required_variable<int>("NUMBER");

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (parsed_and_validated_pre.ok()) {
		const auto name = parsed_and_validated_pre.get_or(name_id, "simple_example");
		const auto number = parsed_and_validated_pre.get(number_id);

		std::cout << "Name  : " << name << std::endl;
		std::cout << "Number: " << number << std::endl;
	} else {
		std::cout << parsed_and_validated_pre.warning_message() << std::endl;
		std::cout << parsed_and_validated_pre.error_message() << std::endl;
	}

	return EXIT_SUCCESS;
}
