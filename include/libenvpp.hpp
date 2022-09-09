#pragma once

#include <algorithm>
#include <any>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <initializer_list>
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

#include <levenshtein.hpp>
#include <libenvpp_env.hpp>
#include <libenvpp_errors.hpp>
#include <libenvpp_parser.hpp>
#include <libenvpp_util.hpp>

namespace env {

class prefix;
template <typename Prefix>
class parsed_and_validated_prefix;

namespace detail {

class variable_data {
  public:
	using parser_and_validator_fn = std::function<std::any(const std::string_view)>;

	variable_data() = delete;

	variable_data(const variable_data&) = delete;
	variable_data(variable_data&&) = default;

	variable_data& operator=(const variable_data&) = delete;
	variable_data& operator=(variable_data&&) = default;

  private:
	variable_data(const std::string_view name, bool is_required, parser_and_validator_fn parser_and_validator)
	    : m_name(name), m_is_required(is_required), m_parser_and_validator(std::move(parser_and_validator))
	{}

	std::string m_name;
	bool m_is_required;
	parser_and_validator_fn m_parser_and_validator;
	std::any m_value;

	friend prefix;
	template <typename Prefix>
	friend class ::env::parsed_and_validated_prefix;
};

} // namespace detail

template <typename T>
struct default_validator {
	void operator()(const T&) const noexcept {}
};

template <typename T>
struct default_parser {
	[[nodiscard]] T operator()(const std::string_view str) const { return detail::construct_from_string<T>(str); }
};

template <typename T>
struct default_parser_and_validator {
	[[nodiscard]] T operator()(const std::string_view str) const
	{
		const auto value = default_parser<T>{}(str);
		default_validator<T>{}(value);
		return value;
	}
};

template <typename T, bool IsRequired>
class variable_id {
  public:
	variable_id() = delete;
	variable_id(const variable_id&) = delete;
	variable_id(variable_id&&) = default;

	variable_id& operator=(const variable_id&) = delete;
	variable_id& operator=(variable_id&&) = default;

	bool operator==(const std::size_t rhs) const noexcept { return m_idx == rhs; }
	bool operator!=(const std::size_t rhs) const noexcept { return !(*this == rhs); }

	friend bool operator==(const std::size_t lhs, const variable_id& rhs) noexcept { return rhs == lhs; }
	friend bool operator!=(const std::size_t lhs, const variable_id& rhs) noexcept { return !(lhs == rhs); }

  private:
	variable_id(const std::size_t idx) : m_idx(idx) {}

	const std::size_t m_idx;

	friend prefix;
	template <typename Prefix>
	friend class parsed_and_validated_prefix;
};

// Templated to resolve mutual dependency.
template <typename Prefix>
class parsed_and_validated_prefix {
  public:
	parsed_and_validated_prefix() = delete;

	parsed_and_validated_prefix(const parsed_and_validated_prefix&) = delete;
	parsed_and_validated_prefix(parsed_and_validated_prefix&&) = default;

	parsed_and_validated_prefix& operator=(const parsed_and_validated_prefix&) = delete;
	parsed_and_validated_prefix& operator=(parsed_and_validated_prefix&&) = default;

	template <typename T, bool IsRequired>
	[[nodiscard]] auto get(const variable_id<T, IsRequired>& var_id)
	{
		const auto& value = m_prefix.m_registered_vars[var_id.m_idx].m_value;
		if constexpr (IsRequired) {
			return std::any_cast<T>(value);
		} else {
			return value.has_value() ? std::optional<T>{std::any_cast<T>(value)} : std::optional<T>{std::nullopt};
		}
	}

	template <typename T, bool IsRequired>
	[[nodiscard]] T get_or(const variable_id<T, IsRequired>& var_id, const T default_value)
	{
		static_assert(!IsRequired, "Default values are not supported on required variables");

		const auto& value = m_prefix.m_registered_vars[var_id.m_idx].m_value;
		return value.has_value() ? std::any_cast<T>(value) : default_value;
	}

	[[nodiscard]] bool ok() const noexcept { return m_errors.empty() && m_warnings.empty(); }
	[[nodiscard]] std::string error_message() const { return message_formatting_helper("error", m_errors); }
	[[nodiscard]] std::string warning_message() const { return message_formatting_helper("warning", m_warnings); }

	[[nodiscard]] const std::vector<error>& errors() const { return m_errors; }
	[[nodiscard]] const std::vector<error>& warnings() const { return m_warnings; }

	[[nodiscard]] std::string help_message() const { return m_prefix.help_message(); }

  private:
	parsed_and_validated_prefix(Prefix&& pre, std::unordered_map<std::string, std::string> environment)
	    : m_prefix(std::move(pre))
	{
		auto unparsed_env_vars = std::vector<std::size_t>{};

		for (std::size_t id = 0; id < m_prefix.m_registered_vars.size(); ++id) {
			auto& var = m_prefix.m_registered_vars[id];
			const auto var_name = get_full_env_var_name(id);
			const auto var_value = pop_from_environment(var_name, environment);
			if (var.m_value.has_value()) {
				// Skip variables set for testing, but consume their environment value if available.
				continue;
			}
			if (!var_value.has_value()) {
				unparsed_env_vars.push_back(id);
			} else {
				try {
					var.m_value = var.m_parser_and_validator(*var_value);
				} catch (const parser_error& e) {
					m_errors.emplace_back(
					    id, var.m_name,
					    fmt::format("Parser error for environment variable '{}': {}", var.m_name, e.what()));
				} catch (const validation_error& e) {
					m_errors.emplace_back(
					    id, var.m_name,
					    fmt::format("Validation error for environment variable '{}': {}", var.m_name, e.what()));
				} catch (const range_error& e) {
					m_errors.emplace_back(
					    id, var.m_name,
					    fmt::format("Range error for environment variable '{}': {}", var.m_name, e.what()));
				} catch (const std::exception& e) {
					m_errors.emplace_back(id, var.m_name,
					                      fmt::format("Failed to parse or validate environment variable '{}' with: {}",
					                                  var.m_name, e.what()));
				} catch (...) {
					m_errors.emplace_back(
					    id, var.m_name,
					    fmt::format("Failed to parse or validate environment variable '{}' with unknown error",
					                var.m_name));
				}
			}
		}

		for (const auto id : unparsed_env_vars) {
			auto& var = m_prefix.m_registered_vars[id];
			const auto var_name = get_full_env_var_name(id);
			const auto similar_var = find_similar_env_var(var_name, environment);
			if (similar_var.has_value()) {
				const auto msg = fmt::format("Unrecognized environment variable '{}' set, did you mean '{}'?",
				                             *similar_var, var_name);
				pop_from_environment(*similar_var, environment);
				if (var.m_is_required) {
					m_errors.emplace_back(id, var.m_name, msg);
				} else {
					m_warnings.emplace_back(id, var.m_name, msg);
				}
			} else if (var.m_is_required) {
				m_errors.emplace_back(id, var.m_name, fmt::format("Environment variable '{}' not set", var.m_name));
			}
		}

		const auto unused_variables = find_unused_env_vars(environment);

		for (const auto& unused_var : unused_variables) {
			m_warnings.emplace_back(-1, "",
			                        fmt::format("Prefix environment variable '{}' specified but unused", unused_var));
		}
	}

	[[nodiscard]] std::string message_formatting_helper(const std::string_view message_type,
	                                                    const std::vector<error>& errors_or_warnings) const
	{
		if (errors_or_warnings.empty()) {
			return fmt::format("No {}s for prefix '{}'", message_type, m_prefix.m_prefix_name);
		}
		auto msg = fmt::format("The following {} {}(s) occurred when parsing and validating prefix '{}':\n",
		                       errors_or_warnings.size(), message_type, m_prefix.m_prefix_name);
		for (std::size_t i = 0; i < errors_or_warnings.size(); ++i) {
			msg += fmt::format("\t{}{}", errors_or_warnings[i].what(), i + 1 < errors_or_warnings.size() ? "\n" : "");
		}
		return msg;
	}

	std::optional<std::string> pop_from_environment(const std::string_view env_var,
	                                                std::unordered_map<std::string, std::string>& environment) const
	{
		const auto var_it = environment.find(std::string(env_var));
		if (var_it == environment.end()) {
			return std::nullopt;
		}
		const auto [_, val] = *var_it;
		environment.erase(var_it);
		return val;
	}

	[[nodiscard]] std::string get_full_env_var_name(const std::size_t var_id) const
	{
		return m_prefix.m_prefix_name + "_" + m_prefix.m_registered_vars[var_id].m_name;
	}

	[[nodiscard]] std::optional<std::string>
	find_similar_env_var(const std::string_view var_name,
	                     const std::unordered_map<std::string, std::string>& environment) const
	{
		if (environment.empty()) {
			return std::nullopt;
		}

		auto edit_distances = std::vector<std::pair<int, std::string>>{};
		std::transform(environment.begin(), environment.end(), std::back_inserter(edit_distances),
		               [this, &var_name](const auto& entry) {
			               return std::pair{
			                   levenshtein::distance(var_name, entry.first, m_prefix.m_edit_distance_cutoff + 1),
			                   entry.first};
		               });
		std::sort(edit_distances.begin(), edit_distances.end(),
		          [](const auto& a, const auto& b) { return a.first < b.first; });
		const auto& [edit_distance, var] = edit_distances.front();
		if (edit_distance <= m_prefix.m_edit_distance_cutoff) {
			return var;
		}

		return std::nullopt;
	}

	[[nodiscard]] std::vector<std::string>
	find_unused_env_vars(const std::unordered_map<std::string, std::string>& environment) const
	{
		auto unused_env_vars = std::vector<std::string>{};
		for (const auto& [var, _] : environment) {
			if (var.find(m_prefix.m_prefix_name) == 0) {
				unused_env_vars.push_back(var);
			}
		}
		return unused_env_vars;
	}

	Prefix m_prefix;
	std::vector<error> m_errors;
	std::vector<error> m_warnings;

	friend prefix;
};

class prefix {
  public:
	prefix() = delete;
	prefix(const std::string_view prefix_name, const int edit_distance_cutoff = 3)
	    : m_prefix_name(prefix_name), m_edit_distance_cutoff(edit_distance_cutoff)
	{}
	prefix(const prefix&) = delete;
	prefix(prefix&&) = default;

	prefix& operator=(const prefix&) = delete;
	prefix& operator=(prefix&&) = default;

	~prefix() = default;

	template <typename T, typename ParserAndValidatorFn = decltype(default_parser_and_validator<T>{})>
	[[nodiscard]] auto register_variable(const std::string_view name,
	                                     ParserAndValidatorFn parser_and_validator = default_parser_and_validator<T>{})
	{
		return registration_helper<T, false>(name, std::move(parser_and_validator));
	}

	template <typename T, typename ParserAndValidatorFn = decltype(default_parser_and_validator<T>{})>
	[[nodiscard]] auto
	register_required_variable(const std::string_view name,
	                           ParserAndValidatorFn parser_and_validator = default_parser_and_validator<T>{})
	{
		return registration_helper<T, true>(name, std::move(parser_and_validator));
	}

	template <typename T>
	[[nodiscard]] auto register_range(const std::string_view name, const T min, const T max)
	{
		return registration_range_helper<T, false>(name, min, max);
	}

	template <typename T>
	[[nodiscard]] auto register_required_range(const std::string_view name, const T min, const T max)
	{
		return registration_range_helper<T, true>(name, min, max);
	}

	template <typename T>
	[[nodiscard]] auto register_option(const std::string_view name, const std::initializer_list<T> options)
	{
		return registration_option_helper<T, false>(name, options);
	}

	template <typename T>
	[[nodiscard]] auto register_required_option(const std::string_view name, const std::initializer_list<T> options)
	{
		return registration_option_helper<T, true>(name, options);
	}

	template <typename T, bool IsRequired>
	void set_for_testing(const variable_id<T, IsRequired>& var_id, const T& value)
	{
		m_registered_vars[var_id.m_idx].m_value = value;
	}

	[[nodiscard]] parsed_and_validated_prefix<prefix>
	parse_and_validate(std::unordered_map<std::string, std::string> environment = detail::get_environment())
	{
		return {std::move(*this), std::move(environment)};
	}

	[[nodiscard]] std::string help_message() const
	{
		if (m_registered_vars.empty()) {
			return fmt::format("There are no supported environment variables for the prefix '{}'", m_prefix_name);
		}
		auto msg = fmt::format("Prefix '{}' supports the following {} environment variable(s):\n", m_prefix_name,
		                       m_registered_vars.size());
		for (std::size_t i = 0; i < m_registered_vars.size(); ++i) {
			const auto& var = m_registered_vars[i];
			msg += fmt::format("\t{}{}{}", var.m_name, var.m_is_required ? " required" : " optional",
			                   i + 1 < m_registered_vars.size() ? "\n" : "");
		}
		return msg;
	}

  private:
	template <typename T, bool IsRequired, typename ParserAndValidatorFn>
	[[nodiscard]] auto registration_helper(const std::string_view name, ParserAndValidatorFn&& parser_and_validator)
	{
		const auto type_erased_parser_and_validator =
		    [parser_and_validator](const std::string_view env_value) -> std::any {
			return parser_and_validator(env_value);
		};
		m_registered_vars.push_back(
		    detail::variable_data{name, IsRequired, std::move(type_erased_parser_and_validator)});
		return variable_id<T, IsRequired>{m_registered_vars.size() - 1};
	}

	template <typename T, bool IsRequired>
	[[nodiscard]] auto registration_range_helper(const std::string_view name, const T min, const T max)
	{
		const auto parser_and_validator = [min, max](const std::string_view str) {
			const auto value = default_parser<T>{}(str);
			default_validator<T>{}(value);
			if (value < min || value > max) {
				throw range_error{fmt::format("Value {} outside of range [{}, {}]", value, min, max)};
			}
			return value;
		};
		return registration_helper<T, IsRequired>(name, std::move(parser_and_validator));
	}

	template <typename T, bool IsRequired>
	[[nodiscard]] auto registration_option_helper(const std::string_view name, const std::initializer_list<T> options)
	{
		const auto options_set = std::set(options.begin(), options.end());
		if (options_set.size() != options.size()) {
			throw duplicate_option{fmt::format("Duplicate option specified for '{}'", name)};
		}
		const auto parser_and_validator = [name = std::string(name),
		                                   options = std::move(options_set)](const std::string_view str) {
			const auto value = default_parser<T>{}(str);
			default_validator<T>{}(value);
			if (std::all_of(options.begin(), options.end(), [&value](const auto& option) { return option != value; })) {
				throw unrecognized_option{fmt::format("Unrecognized option '{}' for '{}'", str, name)};
			}
			return value;
		};
		return registration_helper<T, IsRequired>(name, std::move(parser_and_validator));
	}

	std::string m_prefix_name;
	int m_edit_distance_cutoff;
	std::vector<detail::variable_data> m_registered_vars;

	template <typename Prefix>
	friend class parsed_and_validated_prefix;
};

} // namespace env
