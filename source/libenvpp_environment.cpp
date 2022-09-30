#include <libenvpp/detail/environment.hpp>

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include <libenvpp/detail/levenshtein.hpp>

namespace env::detail {

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

std::optional<std::string> pop_from_environment(const std::string_view env_var,
                                                std::unordered_map<std::string, std::string>& environment)
{
	const auto var_it = environment.find(std::string(env_var));
	if (var_it == environment.end()) {
		return std::nullopt;
	}
	const auto [_, val] = *var_it;
	environment.erase(var_it);
	return val;
}

} // namespace env::detail
