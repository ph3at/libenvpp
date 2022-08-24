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
	variable_data(const std::string_view name, parser_and_validator_fn parser_and_validator)
	    : m_name(name), m_parser_and_validator(std::move(parser_and_validator))
	{}

	std::string m_name;
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
class default_parser {
  public:
	template <typename Validator>
	default_parser(Validator validator) : m_validator(std::move(validator))
	{}

	[[nodiscard]] T operator()(const std::string_view str) const
	{
		T value = detail::construct_from_string<T>(str);
		m_validator(value);
		return value;
	}

  private:
	std::function<void(const T&)> m_validator;
};

template <typename T>
struct default_parser_and_validator {
	[[nodiscard]] T operator()(const std::string_view str) const
	{
		return default_parser<T>{default_validator<T>{}}(str);
	}
};

template <typename Validator>
default_parser(Validator) -> default_parser<
    std::remove_cv_t<std::remove_reference_t<typename detail::util::callable_traits<Validator>::arg0_type>>>;

template <typename T, bool IsRequired>
class variable_id {
  public:
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
		const auto& value = m_prefix.m_registered_vars[var_id.m_idx].m_value;
		return value.has_value() ? std::any_cast<T>(value) : default_value;
	}

	[[nodiscard]] bool ok() const noexcept { return m_errors.empty() && m_warnings.empty(); }
	[[nodiscard]] std::string error_message() const { return {}; }
	[[nodiscard]] std::string warning_message() const { return {}; }

	[[nodiscard]] const std::vector<parser_error>& errors() const { return m_errors; }
	[[nodiscard]] const std::vector<unrecognized_option>& warnings() const { return m_warnings; }

  private:
	parsed_and_validated_prefix(Prefix&& pre) : m_prefix(std::move(pre))
	{
		for (auto& var : m_prefix.m_registered_vars) {
			const auto env_var_name = m_prefix.m_prefix_name + "_" + var.m_name;
			const auto env_var_value = detail::get_environment_variable(env_var_name);
			if (!env_var_value.has_value()) {
				m_errors.emplace_back(fmt::format("Environment variable '{}' not set", env_var_name));
			} else {
				try {
					var.m_value = var.m_parser_and_validator(*env_var_value);
				} catch (const parser_error& e) {
					m_errors.push_back(e);
				} catch (const range_error& e) {
					m_errors.emplace_back(e.what());
				} catch (const std::exception& e) {
					m_errors.emplace_back(
					    fmt::format("Failed to parse or validate environment variable '{}' with error '{}'",
					                env_var_name, e.what()));
				} catch (...) {
					m_errors.emplace_back(
					    fmt::format("Failed to parse or validate environment variable '{}' with error unknown error",
					                env_var_name));
				}
			}
		}
	}

	Prefix m_prefix;
	std::vector<parser_error> m_errors;
	std::vector<unrecognized_option> m_warnings;

	friend prefix;
};

class prefix {
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

	[[nodiscard]] parsed_and_validated_prefix<prefix> parse_and_validate() { return {std::move(*this)}; }

	[[nodiscard]] std::string help_message() const { return {}; }

  private:
	template <typename T, bool IsRequired, typename ParserAndValidatorFn>
	[[nodiscard]] auto registration_helper(const std::string_view name, ParserAndValidatorFn&& parser_and_validator)
	{
		const auto type_erased_parser_and_validator =
		    [parser_and_validator](const std::string_view env_value) -> std::any {
			return parser_and_validator(env_value);
		};
		m_registered_vars.push_back(detail::variable_data{name, std::move(type_erased_parser_and_validator)});
		return variable_id<T, IsRequired>{m_registered_vars.size() - 1};
	}

	template <typename T, bool IsRequired>
	[[nodiscard]] auto registration_range_helper(const std::string_view name, const T min, const T max)
	{
		const auto validator = [min, max](const T& value) {
			default_validator<T>{}(value);
			if (value < min || value > max) {
				throw range_error{fmt::format("Value {} outside of range [{}, {}]", value, min, max)};
			}
		};
		return registration_helper<T, IsRequired>(name, default_parser{std::move(validator)});
	}

	template <typename T, bool IsRequired>
	[[nodiscard]] auto registration_option_helper(const std::string_view name, const std::initializer_list<T> options)
	{
		const auto validator = [options = std::vector(options.begin(), options.end())](const T& value) {
			default_validator<T>{}(value);
			if (std::all_of(options.begin(), options.end(), [&value](const auto& option) { return option != value; })) {
				if constexpr (detail::util::has_to_string_v<T>) {
					using std::to_string;
					throw unrecognized_option{fmt::format("Unrecognized option '{}'", to_string(value))};
				} else {
					throw unrecognized_option{"Unrecognized option"};
				}
			}
		};
		return registration_helper<T, IsRequired>(name, default_parser{std::move(validator)});
	}

	std::string m_prefix_name;
	int m_edit_distance_cutoff;
	std::vector<detail::variable_data> m_registered_vars;

	template <typename Prefix>
	friend class parsed_and_validated_prefix;
};

} // namespace env
