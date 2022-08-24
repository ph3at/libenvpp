#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace env {

class duplicate_option : public std::invalid_argument {
  public:
	duplicate_option() = delete;
	duplicate_option(const std::string_view message) : std::invalid_argument(std::string(message)) {}
};

class parser_error : public std::runtime_error {
  public:
	parser_error() = delete;
	parser_error(const std::string_view message) : std::runtime_error(std::string(message)) {}
};

class range_error : public std::runtime_error {
  public:
	range_error() = delete;
	range_error(const std::string_view message) : std::runtime_error(std::string(message)) {}
};

class unrecognized_option : public std::runtime_error {
  public:
	unrecognized_option() = delete;
	unrecognized_option(const std::string_view message) : std::runtime_error(std::string(message)) {}
};

class error {
  public:
	error() = delete;
	error(const std::size_t var_idx, const std::string_view error_message)
	    : m_var_idx(var_idx), m_error_message(error_message)
	{}

	error(const error&) = delete;
	error(error&&) = default;

	error& operator=(const error&) = delete;
	error& operator=(error&&) = default;

	[[nodiscard]] std::size_t get_id() const noexcept { return m_var_idx; }

	[[nodiscard]] const std::string& what() const noexcept { return m_error_message; }

  private:
	std::size_t m_var_idx;
	std::string m_error_message;
};

} // namespace env
