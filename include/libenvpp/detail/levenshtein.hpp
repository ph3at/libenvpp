#pragma once

#include <limits>
#include <string_view>

namespace levenshtein {

// Checks whether the levenshtein distance between 'lhs' and 'rhs' is (strictly) less than 'cutoff_distance'.
[[nodiscard]] bool is_distance_less_than(std::string_view lhs, std::string_view rhs, const int cutoff_distance);

// Computes the levenshtein distance between 'lhs' and 'rhs', up to a maximum of 'cutoff_distance'.
// Returns the distance, but at most 'cutoff_distance' (i.e. if the actual distance is 5 but the cutoff is 3, 3 will be
// returned).
[[nodiscard]] int distance(std::string_view lhs, std::string_view rhs,
                           const int cutoff_distance = std::numeric_limits<int>::max());

} // namespace levenshtein
