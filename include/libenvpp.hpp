#pragma once

#include <cassert>
#include <cstddef>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace env {

class duplicate_option : public std::invalid_argument {
  public:
	duplicate_option() : std::invalid_argument("Same option specified more than once") {}
};

namespace detail {

enum class variable_type_tag { variable, option };

class empty_base {};

template <typename T, variable_type_tag VarType, template <typename, variable_type_tag> typename Var>
class option_base {
	using base_type = Var<T, VarType>;

  public:
	base_type&& options(const std::initializer_list<T> options_list)
	{
		set_options(options_list);
		return std::move(*static_cast<base_type*>(this));
	}
	void set_options(const std::initializer_list<T> options_list)
	{
		m_options = options_list;
		if (m_options.size() != options_list.size()) {
			throw duplicate_option{};
		}
	}

  protected:
	std::set<T> m_options;
};

template <typename T, template <typename, variable_type_tag> typename Var>
class arithmetic_base {
	using base_type = Var<T, variable_type_tag::variable>;

  public:
	base_type&& range(const T min, const T max) noexcept
	{
		set_range(min, max);
		return std::move(*static_cast<base_type*>(this));
	}
	void set_range(const T min, const T max) noexcept
	{
		m_min = min;
		m_max = max;
	}

  protected:
	T m_min = std::numeric_limits<T>::lowest();
	T m_max = std::numeric_limits<T>::max();
};

} // namespace detail

class prefix;

template <typename T, detail::variable_type_tag VarType>
class var : public std::conditional_t<VarType == detail::variable_type_tag::option, //
                                      detail::option_base<T, VarType, var>,
                                      std::conditional_t<std::is_arithmetic_v<T>,         //
                                                         detail::arithmetic_base<T, var>, //
                                                         detail::empty_base>> {
  public:
	var() = delete;
	var(const var&) = delete;
	var(var&& other) noexcept { *this = std::move(other); }

	var& operator=(const var&) = delete;
	var& operator=(var&& other) noexcept
	{
		m_name = std::move(other.m_name);
		m_id = other.m_id;
		m_prefix_instance = other.m_prefix_instance;
		m_is_required = other.m_is_required;
		other.m_prefix_instance = nullptr;
		return *this;
	}

	~var();

	var&& required() noexcept
	{
		set_required(true);
		return std::move(*this);
	}
	void set_required(const bool is_required) noexcept { m_is_required = is_required; }

	[[nodiscard]] std::optional<T> get() { return {}; }
	[[nodiscard]] T get_or(const T default_value) { return {}; }

	void set_for_testing(const T value) {}

  private:
	var(const std::string_view name, const std::size_t id, prefix& prefix_instance)
	    : m_name(name), m_id(id), m_prefix_instance(&prefix_instance)
	{}

	std::string m_name;
	std::size_t m_id;
	prefix* m_prefix_instance = nullptr;
	bool m_is_required = false;

	friend class prefix;
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

class validation_result {
  public:
	[[nodiscard]] bool ok() const noexcept { return m_errors.empty() && m_warnings.empty(); }
	[[nodiscard]] std::string error_message() const { return {}; }
	[[nodiscard]] std::string warning_message() const { return {}; }

	[[nodiscard]] const std::vector<parser_error>& errors() const { return m_errors; }
	[[nodiscard]] const std::vector<unrecognized_option>& warnings() const { return m_warnings; }

  private:
	std::vector<parser_error> m_errors;
	std::vector<unrecognized_option> m_warnings;
};

class prefix {
  public:
	prefix() = delete;
	prefix(const std::string_view prefix_name, const int edit_distance_cutoff = 2)
	    : m_prefix_name(prefix_name), m_edit_distance_cutoff(edit_distance_cutoff)
	{}
	prefix(const prefix&) = delete;
	prefix(prefix&& other) noexcept { *this = std::move(other); }

	prefix& operator=(const prefix&) = delete;
	prefix& operator=(prefix&& other) noexcept
	{
		m_prefix_name = std::move(other.m_prefix_name);
		m_edit_distance_cutoff = other.m_edit_distance_cutoff;
		m_id_counter = other.m_id_counter;
		m_registered_vars = std::move(other.m_registered_vars);
		return *this;
	}

	~prefix() { assert(m_registered_vars.empty() && "prefix object is being destroyed while var objects still exist"); }

	template <typename T>
	[[nodiscard]] var<T, detail::variable_type_tag::variable> register_variable(const std::string_view name)
	{
		m_registered_vars.insert(m_id_counter);
		return {name, m_id_counter++, *this};
	}

	template <typename T>
	[[nodiscard]] var<T, detail::variable_type_tag::option> register_option(const std::string_view name)
	{
		m_registered_vars.insert(m_id_counter);
		return {name, m_id_counter++, *this};
	}

	[[nodiscard]] validation_result validate() { return {}; }

	[[nodiscard]] std::string help_message() const { return {}; }

  private:
	std::string m_prefix_name;
	int m_edit_distance_cutoff;
	std::size_t m_id_counter = 0;
	std::set<std::size_t> m_registered_vars;

	template <typename T, detail::variable_type_tag VarType>
	friend class var;
};

template <typename T, detail::variable_type_tag VarType>
var<T, VarType>::~var()
{
	if (m_prefix_instance) {
		m_prefix_instance->m_registered_vars.erase(m_id);
	}
}

} // namespace env
