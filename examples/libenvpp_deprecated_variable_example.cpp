#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <libenvpp/env.hpp>

int main()
{
	auto pre = env::prefix("MYPROG");

	pre.register_deprecated("LOG_FILE_PATH", "has been deprecated, use 'MYPROG_LOG_FILE' instead");
	const auto log_file_id = pre.register_variable<std::filesystem::path>("LOG_FILE");

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (parsed_and_validated_pre.ok()) {
		const auto log_file = parsed_and_validated_pre.get_or(log_file_id, "/default/log/file");

		std::cout << "Log file: " << log_file << std::endl;
	} else {
		std::cout << parsed_and_validated_pre.warning_message();
		std::cout << parsed_and_validated_pre.error_message();
	}

	return EXIT_SUCCESS;
}
