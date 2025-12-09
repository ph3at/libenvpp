#include <libenvpp/env.hpp>

#include <fmt/core.h>

int main()
{
	auto pre = env::prefix("TEST");
	const auto var_id = pre.register_variable<std::string>("VAR");
	const auto parsed = pre.parse_and_validate();
	const auto var = parsed.get_or(var_id, "default");
	fmt::print("Integration test passed: {}\n", var);
	return 0;
}
