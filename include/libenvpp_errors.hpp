#pragma once

#include <stdexcept>

namespace env {

class duplicate_option : public std::invalid_argument {
  public:
	duplicate_option() : std::invalid_argument("Same option specified more than once") {}
};

class parser_error : public std::runtime_error {
  public:
	parser_error() = delete;
	parser_error(const std::string_view message) : std::runtime_error(std::string(message)) {}
};

class unrecognized_option : public std::runtime_error {
  public:
	unrecognized_option() = delete;
	unrecognized_option(const std::string_view message) : std::runtime_error(std::string(message)) {}
};

} // namespace env
