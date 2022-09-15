#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include <libenvpp.hpp>

enum class option {
	first_choice,
	second_choice,
	third_choice,
	default_choice,
};

namespace env {
template <>
struct default_parser<option> {
	option operator()(const std::string_view str) const
	{
		auto opt = option{};

		if (str == "first_choice") {
			opt = option::first_choice;
		} else if (str == "second_choice") {
			opt = option::second_choice;
		} else if (str == "third_choice") {
			opt = option::third_choice;
		} else if (str == "default_choice") {
			opt = option::default_choice;
		} else {
			throw parser_error{std::string("Unable to parse '") + std::string(str) + "'"};
		}

		return opt;
	}
};
} // namespace env

int main()
{
	auto pre = env::prefix("OPTION");

	const auto option_id =
	    pre.register_option<option>("CHOICE", {option::first_choice, option::second_choice, option::third_choice});

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (parsed_and_validated_pre.ok()) {
		const auto opt = parsed_and_validated_pre.get_or(option_id, option::default_choice);

		std::cout << "Chosen: ";
		switch (opt) {
		case option::first_choice:
			std::cout << "first_choice" << std::endl;
			break;
		case option::second_choice:
			std::cout << "second_choice" << std::endl;
			break;
		case option::third_choice:
			std::cout << "third_choice" << std::endl;
			break;
		case option::default_choice:
			std::cout << "default_choice" << std::endl;
			break;
		}
	} else {
		std::cout << parsed_and_validated_pre.warning_message();
		std::cout << parsed_and_validated_pre.error_message();
	}

	return EXIT_SUCCESS;
}
