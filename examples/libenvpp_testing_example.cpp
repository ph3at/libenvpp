#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <libenvpp/env.hpp>

int main()
{
	const auto _ = env::scoped_test_environment(std::unordered_map<std::string, std::string>{
	    {"MYPROG_LOG_FILE_PATH", "/dev/null"},
	    {"MYPROG_NUM_THREADS", "8"},
	});

	auto pre = env::prefix("MYPROG");

	const auto log_path_id = pre.register_variable<std::filesystem::path>("LOG_FILE_PATH");
	const auto num_threads_id = pre.register_required_variable<unsigned int>("NUM_THREADS");

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (parsed_and_validated_pre.ok()) {
		const auto log_path = parsed_and_validated_pre.get_or(log_path_id, "/default/log/path");
		const auto num_threads = parsed_and_validated_pre.get(num_threads_id);

		std::cout << "Log path   : " << log_path << std::endl;
		std::cout << "Num threads: " << num_threads << std::endl;
	} else {
		std::cout << parsed_and_validated_pre.warning_message();
		std::cout << parsed_and_validated_pre.error_message();
	}

	return EXIT_SUCCESS;
}
