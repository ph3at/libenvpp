#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <libenvpp.hpp>

int main()
{
	auto pre = env::prefix("MYPROG");

	const auto log_path_id = pre.register_variable<std::filesystem::path>("LOG_FILE_PATH");
	const auto num_threads_id = pre.register_variable<unsigned int>("NUM_THREADS");

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (parsed_and_validated_pre.ok()) {
		const auto log_path = parsed_and_validated_pre.get_or(log_path_id, "/default/log/path");
		const auto num_threads = parsed_and_validated_pre.get_or(num_threads_id, 1);

		std::cout << "Log path   : " << log_path << std::endl;
		std::cout << "Num threads: " << num_threads << std::endl;
	} else {
		std::cout << parsed_and_validated_pre.warning_message() << std::endl;
		std::cout << parsed_and_validated_pre.error_message() << std::endl;
	}

	return EXIT_SUCCESS;
}
