#include <cstdlib>
#include <iostream>
#include <thread>

#include <libenvpp.hpp>

int main()
{
	auto pre = env::prefix("RANGE");

	const auto num_threads_id = pre.register_range<unsigned int>("NUM_THREADS", 1, std::thread::hardware_concurrency());

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (parsed_and_validated_pre.ok()) {
		const auto num_threads = parsed_and_validated_pre.get_or(num_threads_id, std::thread::hardware_concurrency());

		std::cout << "Number of threads: " << num_threads << std::endl;
	} else {
		std::cout << parsed_and_validated_pre.warning_message() << std::endl;
		std::cout << parsed_and_validated_pre.error_message() << std::endl;
	}

	return EXIT_SUCCESS;
}
