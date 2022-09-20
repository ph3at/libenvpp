#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <libenvpp/env.hpp>

static std::vector<std::string> split(const std::string_view str, const char delimiter)
{
	auto result = std::vector<std::string>{};
	auto sstream = std::istringstream(std::string(str));
	auto item = std::string{};
	while (std::getline(sstream, item, delimiter)) {
		result.push_back(std::move(item));
	}
	return result;
}

struct program_data {
	int number;
	float percent;
};

namespace env {
template <>
struct default_parser<program_data> {
	program_data operator()(const std::string_view str) const
	{
		const auto split_str = split(str, ',');
		if (split_str.size() != 2) {
			// Report an error if the input does not have the expected format
			throw parser_error{"Expected 2 comma delimited values"};
		}

		auto parsed_data = program_data{};

		// Delegate parsing of primitive types to the default_parser
		parsed_data.number = default_parser<int>{}(split_str[0]);
		parsed_data.percent = default_parser<float>{}(split_str[1]);

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

		std::cout << "Program data number : " << prog_data.number << std::endl;
		std::cout << "Program data percent: " << prog_data.percent << std::endl;
	} else {
		std::cout << parsed_and_validated_pre.warning_message();
		std::cout << parsed_and_validated_pre.error_message();
	}

	return EXIT_SUCCESS;
}
