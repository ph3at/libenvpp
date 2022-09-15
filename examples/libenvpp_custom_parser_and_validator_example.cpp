#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <libenvpp.hpp>

std::filesystem::path path_parser_and_validator(const std::string_view str)
{
	const auto log_path = std::filesystem::path(str);

	if (!std::filesystem::exists(log_path)) {
		if (!std::filesystem::create_directory(log_path)) {
			throw env::validation_error{"Unable to create log directory"};
		}
	} else if (!std::filesystem::is_directory(log_path)) {
		throw env::validation_error{"Log path is not a directory"};
	}

	return log_path;
}

int main()
{
	auto pre = env::prefix("CUSTOM_PARSER_AND_VALIDATOR");

	const auto path_id = pre.register_required_variable<std::filesystem::path>("LOG_PATH", path_parser_and_validator);

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (parsed_and_validated_pre.ok()) {
		const auto path = parsed_and_validated_pre.get(path_id);

		std::cout << "Logging directory: " << path << std::endl;
	} else {
		std::cout << parsed_and_validated_pre.warning_message();
		std::cout << parsed_and_validated_pre.error_message();
	}

	return EXIT_SUCCESS;
}
