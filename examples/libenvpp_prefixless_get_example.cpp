#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <libenvpp/env.hpp>

int main()
{
	const auto log_path = env::get_or<std::filesystem::path>("LOG_FILE_PATH", "/default/log/path");
	const auto num_threads = env::get<unsigned int>("NUM_THREADS");

	if (num_threads.has_value()) {
		std::cout << "Log path   : " << log_path << std::endl;
		std::cout << "Num threads: " << num_threads.value() << std::endl;
	} else {
		std::cout << num_threads.error().what() << std::endl;
	}

	return EXIT_SUCCESS;
}
