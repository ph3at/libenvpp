#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace env::detail {

std::unordered_map<std::string, std::string> get_environment();

std::optional<std::string> get_environment_variable(const std::string_view name);

void set_environment_variable(const std::string_view name, const std::string_view value);

void delete_environment_variable(const std::string_view name);

class set_scoped_environment_variable {
  public:
	set_scoped_environment_variable() = delete;
	set_scoped_environment_variable(const std::string_view name, const std::string_view value)
	    : m_name(name), m_old_value(get_environment_variable(name))
	{
		set_environment_variable(name, value);
	}

	set_scoped_environment_variable(const set_scoped_environment_variable&) = delete;
	set_scoped_environment_variable(set_scoped_environment_variable&&) = default;

	set_scoped_environment_variable& operator=(const set_scoped_environment_variable&) = delete;
	set_scoped_environment_variable& operator=(set_scoped_environment_variable&&) = default;

	~set_scoped_environment_variable()
	{
		if (m_old_value.has_value()) {
			set_environment_variable(m_name, *m_old_value);
		} else {
			delete_environment_variable(m_name);
		}
	}

  private:
	std::string m_name;
	std::optional<std::string> m_old_value;
};

} // namespace env::detail
