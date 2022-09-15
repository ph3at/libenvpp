#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <libenvpp.hpp>

namespace env {
template <>
struct default_validator<std::filesystem::path> {
	void operator()(const std::filesystem::path& path) const
	{
		if (!std::filesystem::exists(path)) {
			throw validation_error{path.string() + " path does not exist"};
		}
	}
};
} // namespace env

int main()
{
	auto pre = env::prefix("CUSTOM_VALIDATOR");

	const auto path_id = pre.register_variable<std::filesystem::path>("PATH");

	const auto parsed_and_validated_pre = pre.parse_and_validate();

	if (parsed_and_validated_pre.ok()) {
		auto path = parsed_and_validated_pre.get(path_id);

		if (!path.has_value()) {
			path = std::filesystem::path{"logpath"};
			std::filesystem::create_directory(*path);
		}

		std::cout << "Existing logging directory: " << *path << std::endl;
	} else {
		std::cout << parsed_and_validated_pre.warning_message();
		std::cout << parsed_and_validated_pre.error_message();
	}

	return EXIT_SUCCESS;
}
