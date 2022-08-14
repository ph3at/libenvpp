#pragma once

#include <algorithm>
#include <any>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/core.h>

namespace env {

class prefix;

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

namespace detail {

enum class variable_type {
	variable,
	range,
	option,
};

template <typename T, variable_type VariableType, bool IsRequired>
class variable_id {
  public:
	using value_type = T;
	static constexpr auto var_type = VariableType;
	static constexpr auto is_required = IsRequired;

	variable_id() = delete;
	variable_id(const variable_id&) = delete;
	variable_id(variable_id&&) = default;

	variable_id& operator=(const variable_id&) = delete;
	variable_id& operator=(variable_id&&) = default;

  private:
	variable_id(const std::size_t idx) : m_idx(idx) {}

	const std::size_t m_idx;

	friend prefix;
	template <typename Prefix>
	friend class validated_prefix;
};

class variable_data {
  public:
	using parser_fn_type = std::function<std::any(const std::string_view)>;
	using validator_fn_type = std::function<std::string(std::any)>;

	variable_data() = delete;

	variable_data(const variable_data&) = delete;
	variable_data(variable_data&&) = default;

	variable_data& operator=(const variable_data&) = delete;
	variable_data& operator=(variable_data&&) = default;

  private:
	variable_data(const std::string_view name, parser_fn_type parser_fn, validator_fn_type validator_fn)
	    : m_name(name), m_parser_fn(parser_fn), m_validator_fn(validator_fn)
	{}

	const std::string m_name;
	parser_fn_type m_parser_fn;
	validator_fn_type m_validator_fn;
	std::any m_value;

	friend prefix;
	template <typename Prefix>
	friend class validated_prefix;
};

// Templated to resolve mutual dependency
template <typename Prefix>
class validated_prefix {
  public:
	validated_prefix() = delete;

	validated_prefix(const validated_prefix&) = delete;
	validated_prefix(validated_prefix&&) = default;

	validated_prefix& operator=(const validated_prefix&) = delete;
	validated_prefix& operator=(validated_prefix&&) = default;

	template <typename T, detail::variable_type VariableType, bool IsRequired>
	auto get(const detail::variable_id<T, VariableType, IsRequired>& var_id)
	{
		if constexpr (IsRequired) {
			return std::any_cast<T>(m_prefix.m_registered_vars[var_id.m_idx].m_value);
		} else {
			return std::optional<T>{std::any_cast<T>(m_prefix.m_registered_vars[var_id.m_idx].m_value)};
		}
	}

	[[nodiscard]] bool ok() const noexcept { return m_errors.empty() && m_warnings.empty(); }
	[[nodiscard]] std::string error_message() const { return {}; }
	[[nodiscard]] std::string warning_message() const { return {}; }

	[[nodiscard]] const std::vector<parser_error>& errors() const { return m_errors; }
	[[nodiscard]] const std::vector<unrecognized_option>& warnings() const { return m_warnings; }

  private:
	validated_prefix(Prefix&& pre) : m_prefix(std::move(pre))
	{
		for (auto& var : m_prefix.m_registered_vars) {
			const auto env_var_name = m_prefix.m_prefix_name + "_" + var.m_name;
			const auto env_var_value = "7TODO";
			var.m_value = var.m_parser_fn(env_var_value);
			const auto err = var.m_validator_fn(var.m_value);
			if (!err.empty()) {
				m_errors.emplace_back(fmt::format("Variable '{}' with error '{}'", env_var_name, err));
			}
		}
	}

	Prefix m_prefix;
	std::vector<parser_error> m_errors;
	std::vector<unrecognized_option> m_warnings;

	friend prefix;
};

} // namespace detail

class prefix {
	template <typename T>
	static std::string default_variable_validator(const std::optional<T>&)
	{
		return {};
	}

  public:
	prefix() = delete;
	prefix(const std::string_view prefix_name, const int edit_distance_cutoff = 2)
	    : m_prefix_name(prefix_name), m_edit_distance_cutoff(edit_distance_cutoff)
	{}
	prefix(const prefix&) = delete;
	prefix(prefix&&) = default;

	prefix& operator=(const prefix&) = delete;
	prefix& operator=(prefix&&) = default;

	~prefix() = default;

	template <typename T, typename ValidatorFn = decltype(default_variable_validator<T>)>
	[[nodiscard]] auto register_variable(const std::string_view name,
	                                     ValidatorFn validator_fn = default_variable_validator<T>)
	{
		m_registered_vars.push_back(detail::variable_data{name,
		                                                  [](const std::string_view env_value) -> std::any {
			                                                  auto stream_converter =
			                                                      std::istringstream(std::string(env_value));
			                                                  T parsed_value;
			                                                  stream_converter >> parsed_value;
			                                                  if (stream_converter.fail()) {
				                                                  return {};
			                                                  }
			                                                  return {parsed_value};
		                                                  },
		                                                  [validator_fn](std::any variable_value) -> std::string {
			                                                  if (variable_value.has_value()) {
				                                                  return validator_fn(std::any_cast<T>(variable_value));
			                                                  }
			                                                  return validator_fn({});
		                                                  }});
		return detail::variable_id<T, detail::variable_type::variable, false>{m_registered_vars.size() - 1};
	}

	template <typename T, typename ValidatorFn = decltype(default_variable_validator<T>)>
	[[nodiscard]] auto register_required_variable(const std::string_view name,
	                                              ValidatorFn validator_fn = default_variable_validator<T>)
	{
		m_registered_vars.push_back(detail::variable_data{name,
		                                                  [](const std::string_view env_value) -> std::any {
			                                                  auto stream_converter =
			                                                      std::istringstream(std::string(env_value));
			                                                  T parsed_value;
			                                                  stream_converter >> parsed_value;
			                                                  if (stream_converter.fail()) {
				                                                  return {};
			                                                  }
			                                                  return {parsed_value};
		                                                  },
		                                                  [validator_fn](std::any variable_value) -> std::string {
			                                                  if (variable_value.has_value()) {
				                                                  return validator_fn(std::any_cast<T>(variable_value));
			                                                  }
			                                                  return "Missing required value";
		                                                  }});
		return detail::variable_id<T, detail::variable_type::variable, true>{m_registered_vars.size() - 1};
	}

	[[nodiscard]] detail::validated_prefix<prefix> validate() { return {std::move(*this)}; }

	[[nodiscard]] std::string help_message() const { return {}; }

  private:
	std::string m_prefix_name;
	int m_edit_distance_cutoff;
	std::vector<detail::variable_data> m_registered_vars;

	template <typename Prefix>
	friend class detail::validated_prefix;
};

} // namespace env
