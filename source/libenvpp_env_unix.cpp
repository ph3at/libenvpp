#if LIBENVPP_PLATFORM_UNIX

#include <libenvpp_env.hpp>

#include <stdlib.h>

#include <array>

#include <libenvpp_check.hpp>

extern "C" const char* const* const environ;

namespace env::detail {

[[nodiscard]] std::unordered_map<std::string, std::string> get_environment()
{
	auto env_map = std::unordered_map<std::string, std::string>{};

	if (!environ) {
		return env_map;
	}

	for (auto var = environ; *var; ++var) {
		auto var_name_value = std::array<std::string, 2>{};
		auto idx = std::size_t{0};
		for (auto val = *var; *val; ++val) {
			if (idx == 0 && *val == '=') {
				++idx;
			} else {
				var_name_value[idx] += *val;
			}
		}
		env_map[var_name_value[0]] = var_name_value[1];
	}

	return env_map;
}

[[nodiscard]] std::optional<std::string> get_environment_variable(const std::string_view name)
{
	const auto env_var_value = getenv(std::string(name).c_str());
	if (env_var_value == nullptr) {
		return {};
	}
	return std::string(env_var_value);
}

void set_environment_variable(const std::string_view name, const std::string_view value)
{
	const auto env_var_was_set = setenv(std::string(name).c_str(), std::string(value).c_str(), true);
	LIBENVPP_CHECK(env_var_was_set == 0);
}

void delete_environment_variable(const std::string_view name)
{
	const auto env_var_was_deleted = unsetenv(std::string(name).c_str());
	LIBENVPP_CHECK(env_var_was_deleted == 0);
}

} // namespace env::detail

#endif
