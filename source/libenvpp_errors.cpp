#include <libenvpp/detail/errors.hpp>

#include <fmt/core.h>

#include <libenvpp/detail/environment.hpp>

namespace env::detail {

[[nodiscard]] std::optional<error> get_similar_env_var_error(const std::size_t id, const std::string_view env_var_name,
                                                             const int edit_dist_cutoff,
                                                             std::unordered_map<std::string, std::string>& environment)
{
	const auto similar_var = find_similar_env_var(env_var_name, environment, edit_dist_cutoff);
	if (similar_var.has_value()) {
		const auto msg =
		    fmt::format("Unrecognized environment variable '{}' set, did you mean '{}'?", *similar_var, env_var_name);
		pop_from_environment(*similar_var, environment);
		return {error(id, env_var_name, msg)};
	}
	return std::nullopt;
}

[[nodiscard]] error get_unset_env_var_error(const std::size_t id, const std::string_view env_var_name)
{
	return error(id, env_var_name, fmt::format("Environment variable '{}' not set", env_var_name));
}

} // namespace env::detail
