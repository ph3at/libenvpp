#pragma once

#include <cstddef>

namespace env {

class edit_distance {
  public:
	constexpr edit_distance() : m_value(-1) {}
	constexpr explicit edit_distance(const int value) : m_value(value) {}

	constexpr edit_distance(const edit_distance&) = default;
	constexpr edit_distance(edit_distance&&) = default;

	constexpr edit_distance& operator=(const edit_distance&) = default;
	constexpr edit_distance& operator=(edit_distance&&) = default;

	[[nodiscard]] constexpr int get_or_default(const std::size_t length) const
	{
		if (m_value >= 0) {
			return m_value;
		} else if (length <= 3) {
			return 0;
		} else if (length <= 6) {
			return 1;
		} else if (length <= 9) {
			return 2;
		} else {
			return 3;
		}
	}

  private:
	int m_value;
};

inline constexpr auto default_edit_distance = edit_distance();

} // namespace env
