#pragma once

#include <cstddef>
#include <string_view>
#include <utility>

#include <libenvpp/detail/edit_distance.hpp>
#include <libenvpp/detail/environment.hpp>
#include <libenvpp/detail/errors.hpp>
#include <libenvpp/detail/expected.hpp>
#include <libenvpp/detail/parser.hpp>
#include <libenvpp/detail/testing.hpp>

namespace env {

template <typename T>
[[nodiscard]] expected<T, error> get(const std::string_view env_var_name,
                                     const edit_distance edit_distance_cutoff = default_edit_distance)
{
	using expected_t = expected<T, error>;
	using unexpected_t = typename expected_t::unexpected_type;

	// Merges the global testing environment into the environment considered for parsing and validating,
	// giving precedence to variables set in the testing environment.
	auto environment = detail::merge_environments(detail::g_testing_environment, detail::get_environment());

	if (const auto env_var_it = environment.find(std::string(env_var_name)); env_var_it != environment.end()) {
		auto res = detail::parse_or_error<T>(env_var_name, env_var_it->second, default_parser_and_validator<T>{});
		if (res.has_value()) {
			return expected_t{std::move(res).value()};
		}
		return expected_t{unexpected_t{error(static_cast<std::size_t>(-1), env_var_name, std::move(res).error())}};
	}

	const auto id = static_cast<std::size_t>(-1);
	const auto edit_dist_cutoff = edit_distance_cutoff.get_or_default(env_var_name.length());
	auto similar_env_var_error = detail::get_similar_env_var_error(id, env_var_name, edit_dist_cutoff, environment);
	if (similar_env_var_error.has_value()) {
		return expected_t{unexpected_t{std::move(similar_env_var_error).value()}};
	}
	return expected_t{unexpected_t{detail::get_unset_env_var_error(id, env_var_name)}};
}

template <typename T, typename U = T>
[[nodiscard]] T get_or(const std::string_view env_var_name, U&& default_value)
{
	// Merges the global testing environment into the environment considered for parsing and validating,
	// giving precedence to variables set in the testing environment.
	const auto environment = detail::merge_environments(detail::g_testing_environment, detail::get_environment());

	if (const auto env_var_it = environment.find(std::string(env_var_name)); env_var_it != environment.end()) {
		auto res = detail::parse_or_error<T>(env_var_name, env_var_it->second, default_parser_and_validator<T>{});
		if (res.has_value()) {
			return std::move(res).value();
		}
	}
	return static_cast<T>(std::forward<U>(default_value));
}

} // namespace env
