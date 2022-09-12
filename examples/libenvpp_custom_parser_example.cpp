#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include <libenvpp.hpp>

struct program_data {
	std::string as_string;
	int as_number;
};

namespace env {
template <>
struct default_parser<program_data> {
	program_data operator()(const std::string_view str) const
	{
		auto parsed_data = program_data{};
		parsed_data.as_string = default_parser<std::string>{}(str);
		parsed_data.as_number = default_parser<int>{}(str);
		return parsed_data;
	}
};
} // namespace env

int main()
{
	auto pre = env::prefix("CUSTOM_PARSER");

	const auto program_data_id = pre.register_required_variable<program_data>("PROGRAM_DATA");

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (parsed_and_validated_pre.ok()) {
		const auto prog_data = parsed_and_validated_pre.get(program_data_id);

		std::cout << "Program data as string: " << prog_data.as_string << std::endl;
		std::cout << "Program data as number: " << prog_data.as_number << std::endl;
	} else {
		std::cout << parsed_and_validated_pre.warning_message() << std::endl;
		std::cout << parsed_and_validated_pre.error_message() << std::endl;
	}

	return EXIT_SUCCESS;
}
