#pragma once

#include <algorithm>
#include <exception>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <libenvpp/detail/edit_distance.hpp>
#include <libenvpp/detail/errors.hpp>
#include <libenvpp/detail/expected.hpp>
#include <libenvpp/detail/levenshtein.hpp>
#include <libenvpp/detail/parser.hpp>

namespace env {

namespace detail {

[[nodiscard]] std::optional<std::string>
find_similar_env_var(const std::string_view var_name, const std::unordered_map<std::string, std::string>& environment,
                     const int edit_distance_cutoff)
{
	if (environment.empty()) {
		return std::nullopt;
	}

	auto edit_distances = std::vector<std::pair<int, std::string>>{};
	std::transform(
	    environment.begin(), environment.end(), std::back_inserter(edit_distances),
	    [&var_name, &edit_distance_cutoff](const auto& entry) {
		    return std::pair{levenshtein::distance(var_name, entry.first, edit_distance_cutoff + 1), entry.first};
	    });
	std::sort(edit_distances.begin(), edit_distances.end(),
	          [](const auto& a, const auto& b) { return a.first < b.first; });
	const auto& [edit_dist, var] = edit_distances.front();
	if (edit_dist <= edit_distance_cutoff) {
		return var;
	}

	return std::nullopt;
}

template <typename T, typename ParserAndValidator>
[[nodiscard]] expected<T, std::string> parse_or_error(const std::string_view env_var_name,
                                                      const std::string_view env_var_value,
                                                      ParserAndValidator&& parser_and_validator)
{
	using expected_t = expected<T, std::string>;
	using unexpected_t = typename expected_t::unexpected_type;

	auto error_msg = typename expected_t::error_type();

	try {
		return expected_t{parser_and_validator(env_var_value)};
	} catch (const parser_error& e) {
		error_msg = fmt::format("Parser error for environment variable '{}': {}", env_var_name, e.what());
	} catch (const validation_error& e) {
		error_msg = fmt::format("Validation error for environment variable '{}': {}", env_var_name, e.what());
	} catch (const range_error& e) {
		error_msg = fmt::format("Range error for environment variable '{}': {}", env_var_name, e.what());
	} catch (const option_error& e) {
		error_msg = fmt::format("Option error for environment variable '{}': {}", env_var_name, e.what());
	} catch (const std::exception& e) {
		error_msg =
		    fmt::format("Failed to parse or validate environment variable '{}' with: {}", env_var_name, e.what());
	} catch (...) {
		error_msg =
		    fmt::format("Failed to parse or validate environment variable '{}' with unknown error", env_var_name);
	}
	return expected_t{unexpected_t{error_msg}};
}

} // namespace detail

template <typename T>
[[nodiscard]] expected<T, error> get(const std::string_view env_var_name,
                                     const edit_distance edit_distance_cutoff = default_edit_distance)
{
	using expected_t = expected<T, error>;
	using unexpected_t = typename expected_t::unexpected_type;

	if (const auto env_var_value = detail::get_environment_variable(env_var_name); env_var_value.has_value()) {
		auto res = detail::parse_or_error<T>(env_var_name, *env_var_value, default_parser_and_validator<T>{});
		if (res.has_value()) {
			return expected_t{std::move(res).value()};
		}
		return expected_t{unexpected_t{error(static_cast<std::size_t>(-1), env_var_name, std::move(res).error())}};
	}

	// TODO: Deduplicate

	const auto edit_dist_cutoff = edit_distance_cutoff.get_or_default(env_var_name.length());
	const auto similar_var = detail::find_similar_env_var(env_var_name, detail::get_environment(), edit_dist_cutoff);
	if (similar_var.has_value()) {
		const auto msg =
		    fmt::format("Unrecognized environment variable '{}' set, did you mean '{}'?", *similar_var, env_var_name);
		return expected_t{unexpected_t{error(static_cast<std::size_t>(-1), env_var_name, msg)}};
	}
	return expected_t{unexpected_t{error(static_cast<std::size_t>(-1), env_var_name,
	                                     fmt::format("Environment variable '{}' not set", env_var_name))}};
}

} // namespace env
