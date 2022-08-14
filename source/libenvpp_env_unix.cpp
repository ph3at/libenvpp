#if LIBENVPP_PLATFORM_UNIX

#include <libenvpp_env.hpp>

namespace env::detail {

std::unordered_map<std::string, std::string> get_environment()
{
	return {};
}

std::optional<std::string> get_environment_variable(const std::string_view name)
{
	return {};
}

void set_environment_variable(const std::string_view name, const std::string_view value) {}

void delete_environment_variable(const std::string_view name) {}

} // namespace env::detail

#endif
