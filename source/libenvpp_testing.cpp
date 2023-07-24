#include <libenvpp/detail/testing.hpp>

#include <fmt/core.h>

#include <libenvpp/detail/errors.hpp>

namespace env {

namespace detail {

std::unordered_map<std::string, std::string> g_testing_environment;

[[nodiscard]] std::unordered_map<std::string, std::string>
merge_environments(const std::unordered_map<std::string, std::string>& high_precedence_env,
                   const std::unordered_map<std::string, std::string>& low_precedence_env)
{
	auto merged = low_precedence_env;
	for (const auto& [name, value] : high_precedence_env) {
		merged[name] = value;
	}
	return merged;
}

} // namespace detail

scoped_test_environment::scoped_test_environment(const std::unordered_map<std::string, std::string>& environment)
    : m_environment(environment)
{
	for (const auto& [name, value] : m_environment) {
		if (const auto it = detail::g_testing_environment.find(name); it != detail::g_testing_environment.end()) {
			throw test_environment_error{fmt::format("The global test environment already contains the value '{}' for "
			                                         "variable '{}', while trying to set it to '{}'",
			                                         it->second, name, value)};
		}
		detail::g_testing_environment[name] = value;
	}
}

scoped_test_environment::scoped_test_environment(const std::string_view name, const std::string_view value)
    : scoped_test_environment(std::unordered_map<std::string, std::string>{{std::string(name), std::string(value)}})
{
}

scoped_test_environment::~scoped_test_environment()
{
	for (const auto& [name, _] : m_environment) {
		detail::g_testing_environment.erase(name);
	}
}

} // namespace env
