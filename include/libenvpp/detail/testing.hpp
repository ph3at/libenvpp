#pragma once

#include <string>
#include <unordered_map>

namespace env {

namespace detail {

extern std::unordered_map<std::string, std::string> g_testing_environment;

[[nodiscard]] std::unordered_map<std::string, std::string>
merge_environments(const std::unordered_map<std::string, std::string>& high_precedence_env,
                   const std::unordered_map<std::string, std::string>& low_precedence_env);

} // namespace detail

class [[nodiscard]] scoped_test_environment {
  public:
	scoped_test_environment(const std::unordered_map<std::string, std::string>& environment);
	~scoped_test_environment();

  private:
	const std::unordered_map<std::string, std::string> m_environment;
};

} // namespace env
