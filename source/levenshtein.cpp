#include "levenshtein.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

namespace levenshtein {

bool is_distance_less_than(std::string_view lhs, std::string_view rhs, const int cutoff_distance)
{
	return distance(lhs, rhs, cutoff_distance) < cutoff_distance;
}

// Based on the Apache Commons implementation.
int distance(std::string_view lhs, std::string_view rhs,
             const int cutoff_distance /*= std::numeric_limits<int>::max()*/)
{
	if (cutoff_distance < 0) {
		throw invalid_cutoff{};
	}

	auto lhs_size = static_cast<int>(lhs.length());
	auto rhs_size = static_cast<int>(rhs.length());

	// Early exit for the empty string case.
	if (lhs_size == 0) {
		return rhs_size;
	} else if (rhs_size == 0) {
		return lhs_size;
	}

	// Swap so that lhs is always smaller.
	if (lhs_size > rhs_size) {
		std::swap(lhs, rhs);
		std::swap(lhs_size, rhs_size);
	}

	std::vector<int> p(lhs_size + 1); // Previous cost, horizontally.
	std::vector<int> d(lhs_size + 1); // Cost, horizontally.

	// Fill in starting table values.
	const auto boundary = std::min(lhs_size, cutoff_distance) + 1;
	for (int i = 0; i < boundary; ++i) {
		p[i] = i;
	}
	// These fills ensure that the value above the rightmost entry of our
	// stripe will be ignored in following loop iterations.
	std::fill(p.begin() + boundary, p.end(), std::numeric_limits<int>::max());
	std::fill(d.begin(), d.end(), std::numeric_limits<int>::max());

	// Iterates through rhs.
	for (int j = 1; j <= rhs_size; ++j) {
		d[0] = j;

		// Compute stripe indices, constrain to array size.
		const auto min = std::max(1, j - cutoff_distance);
		const auto max = (j > std::numeric_limits<int>::max() - cutoff_distance)
		                     ? lhs_size
		                     : std::min(lhs_size, j + cutoff_distance);

		// The stripe may lead off of the table if lhs and rhs are of different sizes.
		if (min > max) {
			return cutoff_distance;
		}

		// Ignore entry left of leftmost.
		if (min > 1) {
			d[min - 1] = std::numeric_limits<int>::max();
		}

		// Iterates through [min, max] in lhs.
		for (int i = min; i <= max; ++i) {
			if (lhs[i - 1] == rhs[j - 1]) {
				// Diagonally left and up.
				d[i] = p[i - 1];
			} else {
				// 1 + minimum of cell to the left, to the top, diagonally left and up.
				d[i] = 1 + std::min(std::min(d[i - 1], p[i]), p[i - 1]);
			}
		}

		// Copy current distance counts to 'previous row' distance counts.
		std::swap(p, d);
	}

	return p[lhs_size] <= cutoff_distance ? p[lhs_size] : cutoff_distance;
}

} // namespace levenshtein
