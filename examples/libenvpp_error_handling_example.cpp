#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <libenvpp.hpp>

int main()
{
	auto pre = env::prefix("MYPROG");

	const auto log_path_id = pre.register_variable<std::filesystem::path>("LOG_FILE_PATH");
	const auto num_threads_id = pre.register_required_variable<unsigned int>("NUM_THREADS");

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (!parsed_and_validated_pre.ok()) {
		for (const auto& warning : parsed_and_validated_pre.warnings()) {
			if (warning.get_id() == log_path_id) {
				std::cout << "Log path warning for " << warning.get_name() << ": " << warning.what() << std::endl;
			} else if (warning.get_id() == num_threads_id) {
				std::cout << "Num threads warning for " << warning.get_name() << ": " << warning.what() << std::endl;
			}
		}

		for (const auto& error : parsed_and_validated_pre.errors()) {
			if (error.get_id() == log_path_id) {
				std::cout << "Log path error for " << error.get_name() << ": " << error.what() << std::endl;
			} else if (error.get_id() == num_threads_id) {
				std::cout << "Num thread error for " << error.get_name() << ": " << error.what() << std::endl;
			}
		}

		std::cout << parsed_and_validated_pre.warning_message() << std::endl;
		std::cout << parsed_and_validated_pre.error_message() << std::endl;
	}

	return EXIT_SUCCESS;
}
