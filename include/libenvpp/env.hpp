#pragma once

#include <algorithm>
#include <any>
#include <cstddef>
#include <exception>
#include <functional>
#include <initializer_list>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/core.h>

#include <libenvpp/detail/edit_distance.hpp>
#include <libenvpp/detail/environment.hpp>
#include <libenvpp/detail/errors.hpp>
#include <libenvpp/detail/levenshtein.hpp>
#include <libenvpp/detail/parser.hpp>

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
	variable_id(const variable_id&) = default;
	variable_id(variable_id&&) = delete;

	variable_id& operator=(const variable_id&) = default;
	variable_id& operator=(variable_id&&) = delete;

	bool operator==(const std::size_t rhs) const noexcept { return m_idx == rhs; }
	bool operator!=(const std::size_t rhs) const noexcept { return !(*this == rhs); }

	friend bool operator==(const std::size_t lhs, const variable_id& rhs) noexcept { return rhs == lhs; }
	friend bool operator!=(const std::size_t lhs, const variable_id& rhs) noexcept { return !(lhs == rhs); }

  private:
	variable_id(const std::size_t idx) : m_idx(idx) {}

	std::size_t m_idx;

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
	parsed_and_validated_prefix(parsed_and_validated_prefix&& other) noexcept { *this = std::move(other); }

	parsed_and_validated_prefix& operator=(const parsed_and_validated_prefix&) = delete;
	parsed_and_validated_prefix& operator=(parsed_and_validated_prefix&& other) noexcept
	{
		m_prefix = std::move(other.m_prefix);
		m_errors = std::move(other.m_errors);
		m_warnings = std::move(other.m_warnings);
		m_invalidated = std::move(other.m_invalidated);
		other.m_invalidated = true;
		return *this;
	}

	template <typename T, bool IsRequired>
	[[nodiscard]] auto get(const variable_id<T, IsRequired>& var_id) const
	{
		throw_if_invalid();

		const auto& value = m_prefix.m_registered_vars[var_id.m_idx].m_value;
		if constexpr (IsRequired) {
			if (!value.has_value()) {
				throw value_error{fmt::format("Variable '{}' does not hold a value",
				                              m_prefix.m_registered_vars[var_id.m_idx].m_name)};
			}
			return std::any_cast<T>(value);
		} else {
			return value.has_value() ? std::optional<T>{std::any_cast<T>(value)} : std::optional<T>{std::nullopt};
		}
	}

	template <typename T, bool IsRequired, typename U = T>
	[[nodiscard]] T get_or(const variable_id<T, IsRequired>& var_id, const U& default_value) const
	{
		static_assert(!IsRequired, "Default values are not supported on required variables");

		throw_if_invalid();

		const auto& value = m_prefix.m_registered_vars[var_id.m_idx].m_value;
		return value.has_value() ? std::any_cast<T>(value) : default_value;
	}

	[[nodiscard]] bool ok() const
	{
		throw_if_invalid();
		return m_errors.empty() && m_warnings.empty();
	}

	[[nodiscard]] std::string error_message() const
	{
		throw_if_invalid();
		return message_formatting_helper("Error", m_errors);
	}

	[[nodiscard]] std::string warning_message() const
	{
		throw_if_invalid();
		return message_formatting_helper("Warning", m_warnings);
	}

	[[nodiscard]] const std::vector<error>& errors() const
	{
		throw_if_invalid();
		return m_errors;
	}

	[[nodiscard]] const std::vector<error>& warnings() const
	{
		throw_if_invalid();
		return m_warnings;
	}

	[[nodiscard]] std::string help_message() const
	{
		throw_if_invalid();
		return m_prefix.help_message();
	}

  private:
	void throw_if_invalid() const
	{
		if (m_invalidated) {
			throw invalidated_prefix{"Parsed and validated prefix has been invalidated by moving from it"};
		}
	}

	parsed_and_validated_prefix(Prefix&& pre, std::unordered_map<std::string, std::string> environment)
	    : m_prefix(std::move(pre))
	{
		auto unparsed_env_vars = std::vector<std::size_t>{};

		for (std::size_t id = 0; id < m_prefix.m_registered_vars.size(); ++id) {
			auto& var = m_prefix.m_registered_vars[id];
			const auto var_name = m_prefix.get_full_env_var_name(id);
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
					    fmt::format("Parser error for environment variable '{}': {}", var_name, e.what()));
				} catch (const validation_error& e) {
					m_errors.emplace_back(
					    id, var_name,
					    fmt::format("Validation error for environment variable '{}': {}", var_name, e.what()));
				} catch (const range_error& e) {
					m_errors.emplace_back(
					    id, var_name, fmt::format("Range error for environment variable '{}': {}", var_name, e.what()));
				} catch (const option_error& e) {
					m_errors.emplace_back(
					    id, var_name,
					    fmt::format("Option error for environment variable '{}': {}", var_name, e.what()));
				} catch (const std::exception& e) {
					m_errors.emplace_back(id, var_name,
					                      fmt::format("Failed to parse or validate environment variable '{}' with: {}",
					                                  var_name, e.what()));
				} catch (...) {
					m_errors.emplace_back(
					    id, var_name,
					    fmt::format("Failed to parse or validate environment variable '{}' with unknown error",
					                var_name));
				}
			}
		}

		for (const auto id : unparsed_env_vars) {
			auto& var = m_prefix.m_registered_vars[id];
			const auto var_name = m_prefix.get_full_env_var_name(id);
			const auto similar_var = find_similar_env_var(var_name, environment);
			if (similar_var.has_value()) {
				const auto msg = fmt::format("Unrecognized environment variable '{}' set, did you mean '{}'?",
				                             *similar_var, var_name);
				pop_from_environment(*similar_var, environment);
				if (var.m_is_required) {
					m_errors.emplace_back(id, var_name, msg);
				} else {
					m_warnings.emplace_back(id, var_name, msg);
				}
			} else if (var.m_is_required) {
				m_errors.emplace_back(id, var_name, fmt::format("Environment variable '{}' not set", var_name));
			}
		}

		const auto unused_variables = find_unused_env_vars(environment);

		for (const auto& unused_var : unused_variables) {
			m_warnings.emplace_back(-1, unused_var,
			                        fmt::format("Prefix environment variable '{}' specified but unused", unused_var));
		}
	}

	[[nodiscard]] std::string message_formatting_helper(const std::string_view message_type,
	                                                    const std::vector<error>& errors_or_warnings) const
	{
		if (errors_or_warnings.empty()) {
			return {};
		}
		auto msg = std::string();
		for (std::size_t i = 0; i < errors_or_warnings.size(); ++i) {
			msg += fmt::format("{:<7}: {}\n", message_type, errors_or_warnings[i].what());
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

	[[nodiscard]] std::optional<std::string>
	find_similar_env_var(const std::string_view var_name,
	                     const std::unordered_map<std::string, std::string>& environment) const
	{
		if (environment.empty()) {
			return std::nullopt;
		}

		const auto edit_dist_cutoff = m_prefix.m_edit_distance_cutoff.get_or_default(var_name.length());

		auto edit_distances = std::vector<std::pair<int, std::string>>{};
		std::transform(
		    environment.begin(), environment.end(), std::back_inserter(edit_distances),
		    [&var_name, &edit_dist_cutoff](const auto& entry) {
			    return std::pair{levenshtein::distance(var_name, entry.first, edit_dist_cutoff + 1), entry.first};
		    });
		std::sort(edit_distances.begin(), edit_distances.end(),
		          [](const auto& a, const auto& b) { return a.first < b.first; });
		const auto& [edit_dist, var] = edit_distances.front();
		if (edit_dist <= edit_dist_cutoff) {
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
	bool m_invalidated = false;

	friend prefix;
};

class prefix {
	static constexpr auto PREFIX_DELIMITER = '_';

  public:
	prefix(const std::string_view prefix_name, const edit_distance edit_distance_cutoff = unset_edit_distance)
	    : m_prefix_name(std::string(prefix_name) + PREFIX_DELIMITER), m_edit_distance_cutoff(edit_distance_cutoff)
	{
		if (m_prefix_name.size() == 1 && m_prefix_name[0] == PREFIX_DELIMITER) {
			throw invalid_prefix{"Prefix name must not be empty"};
		}
	}

	prefix(const prefix&) = delete;
	prefix(prefix&& other) noexcept { *this = std::move(other); }

	prefix& operator=(const prefix&) = delete;
	prefix& operator=(prefix&& other) noexcept
	{
		m_prefix_name = std::move(other.m_prefix_name);
		m_edit_distance_cutoff = std::move(other.m_edit_distance_cutoff);
		m_registered_vars = std::move(other.m_registered_vars);
		m_invalidated = std::move(other.m_invalidated);
		other.m_invalidated = true;
		return *this;
	}

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
		throw_if_invalid();
		m_registered_vars[var_id.m_idx].m_value = value;
	}

	[[nodiscard]] parsed_and_validated_prefix<prefix>
	parse_and_validate(std::unordered_map<std::string, std::string> environment = detail::get_environment())
	{
		throw_if_invalid();
		return {std::move(*this), std::move(environment)};
	}

	[[nodiscard]] std::string help_message() const
	{
		throw_if_invalid();

		if (m_registered_vars.empty()) {
			return fmt::format("There are no supported environment variables for the prefix '{}'\n", m_prefix_name);
		}
		auto msg = fmt::format("Prefix '{}' supports the following {} environment variable(s):\n", m_prefix_name,
		                       m_registered_vars.size());
		for (std::size_t i = 0; i < m_registered_vars.size(); ++i) {
			const auto& var = m_registered_vars[i];
			const auto var_name = get_full_env_var_name(i);
			msg += fmt::format("\t'{}' {}\n", var_name, var.m_is_required ? "required" : "optional");
		}
		return msg;
	}

  private:
	prefix() = default;

	[[nodiscard]] std::string get_full_env_var_name(const std::size_t var_id) const
	{
		return get_full_env_var_name(m_registered_vars[var_id].m_name);
	}

	[[nodiscard]] std::string get_full_env_var_name(const std::string_view name) const
	{
		return m_prefix_name + std::string(name);
	}

	void throw_if_invalid() const
	{
		if (m_invalidated) {
			throw invalidated_prefix{
			    "Prefix has been invalidated by either moving from it or by parsing and validating it"};
		}
	}

	template <typename T, bool IsRequired, typename ParserAndValidatorFn>
	[[nodiscard]] auto registration_helper(const std::string_view name, ParserAndValidatorFn&& parser_and_validator)
	{
		throw_if_invalid();

		const auto type_erased_parser_and_validator =
		    [parser_and_validator](const std::string_view env_value) -> std::any {
			static_assert(std::is_convertible_v<decltype(parser_and_validator(env_value)), T>,
			              "Parser and validator function must return type convertible to T");
			return parser_and_validator(env_value);
		};
		m_registered_vars.push_back(
		    detail::variable_data{name, IsRequired, std::move(type_erased_parser_and_validator)});
		return variable_id<T, IsRequired>{m_registered_vars.size() - 1};
	}

	template <typename T, bool IsRequired>
	[[nodiscard]] auto registration_range_helper(const std::string_view name, const T min, const T max)
	{
		if (min > max) {
			throw invalid_range{fmt::format("Invalid range [{}, {}] for '{}', min must be less or equal to max", min,
			                                max, get_full_env_var_name(name))};
		}

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
		if (options.size() == 0) {
			throw empty_option{fmt::format("No options provided for '{}'", get_full_env_var_name(name))};
		}

		const auto options_set = std::set(options.begin(), options.end());
		if (options_set.size() != options.size()) {
			throw duplicate_option{fmt::format("Duplicate option specified for '{}'", get_full_env_var_name(name))};
		}
		const auto parser_and_validator = [options = std::move(options_set)](const std::string_view str) {
			const auto value = default_parser<T>{}(str);
			default_validator<T>{}(value);
			if (std::all_of(options.begin(), options.end(), [&value](const auto& option) { return option != value; })) {
				throw option_error{fmt::format("Unrecognized option '{}'", str)};
			}
			return value;
		};
		return registration_helper<T, IsRequired>(name, std::move(parser_and_validator));
	}

	std::string m_prefix_name;
	edit_distance m_edit_distance_cutoff;
	std::vector<detail::variable_data> m_registered_vars;
	bool m_invalidated = false;

	template <typename Prefix>
	friend class parsed_and_validated_prefix;
};

template <typename T>
[[nodiscard]] std::optional<T> get(const std::string_view env_var_name) noexcept
{
	try {
		if (const auto env_var_value = detail::get_environment_variable(env_var_name); env_var_value.has_value()) {
			return default_parser_and_validator<T>{}(*env_var_value);
		}
	} catch (...) {
	}

	return {};
}

} // namespace env
