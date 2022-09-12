#include <cstdlib>
#include <iostream>

#include <libenvpp.hpp>

namespace env {
template <>
struct default_validator<int> {
	void operator()(const int value) const
	{
		if (value == 42) {
			throw validation_error{"Value 42 is not allowed"};
		}
	}
};
} // namespace env

int main()
{
	auto pre = env::prefix("CUSTOM_VALIDATOR");

	const auto number_id = pre.register_required_variable<int>("NUMBER");

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (parsed_and_validated_pre.ok()) {
		const auto number = parsed_and_validated_pre.get(number_id);

		std::cout << "Number that is not the answer to the ultimate question of life, the universe, and everything: "
		          << number << std::endl;
	} else {
		std::cout << parsed_and_validated_pre.warning_message() << std::endl;
		std::cout << parsed_and_validated_pre.error_message() << std::endl;
	}

	return EXIT_SUCCESS;
}
