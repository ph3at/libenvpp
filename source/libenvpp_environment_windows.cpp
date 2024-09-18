#if LIBENVPP_PLATFORM_WINDOWS

#include <libenvpp/detail/environment.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <array>

#include <libenvpp/detail/check.hpp>

namespace env::detail {

std::optional<std::string> convert_string(const std::wstring& str)
{
	if (str.empty()) {
		return std::string();
	}
	const auto buffer_size =
	    WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0, nullptr, nullptr);
	if (buffer_size == 0) {
		return {};
	}
	auto buffer = std::string(buffer_size, '\0');
	[[maybe_unused]] const auto res = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()),
	                                                      buffer.data(), buffer_size, nullptr, nullptr);
	LIBENVPP_CHECK(res == buffer_size);
	return buffer;
}

std::optional<std::wstring> convert_string(const std::string& str)
{
	if (str.empty()) {
		return std::wstring();
	}
	const auto buffer_size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
	if (buffer_size == 0) {
		return {};
	}
	auto buffer = std::wstring(buffer_size, L'\0');
	[[maybe_unused]] const auto res =
	    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), buffer.data(), buffer_size);
	LIBENVPP_CHECK(res == buffer_size);
	return buffer;
}

[[nodiscard]] std::unordered_map<std::string, std::string> get_environment()
{
	auto env_map = std::unordered_map<std::string, std::string>{};

	const auto environment = GetEnvironmentStringsW();
	if (!environment) {
		return env_map;
	}

	for (const auto* var = environment; *var; ++var) {
		auto var_name_value = std::array<std::wstring, 2>{};
		auto idx = std::size_t{0};
		for (; *var; ++var) {
			if (idx == 0 && *var == L'=') {
				++idx;
			} else {
				var_name_value[idx] += *var;
			}
		}
		if (!var_name_value[0].empty()) {
			auto key = convert_string(var_name_value[0]);
			auto value = convert_string(var_name_value[1]);
			if (key && value) {
				env_map[*key] = *value;
			}
		}
	}

	[[maybe_unused]] const auto env_strings_were_freed = FreeEnvironmentStringsW(environment);
	LIBENVPP_CHECK(env_strings_were_freed);

	return env_map;
}

[[nodiscard]] std::optional<std::string> get_environment_variable(const std::string_view name)
{
	const auto var_name = convert_string(std::string(name));
	if (!var_name) {
		return {};
	}
	const auto buffer_size = GetEnvironmentVariableW(var_name->c_str(), nullptr, 0);
	if (buffer_size == 0) {
		return {};
	}
	// -1 because std::string already contains implicit null terminator
	auto value = std::wstring(buffer_size - 1, L'\0');
	[[maybe_unused]] const auto env_var_got = GetEnvironmentVariableW(var_name->c_str(), value.data(), buffer_size);
	// An empty string will have buffer_size == 1 and thus read 0, which is not an error.
	LIBENVPP_CHECK(env_var_got != 0 || buffer_size == 1);
	return convert_string(value);
}

void set_environment_variable(const std::string_view name, const std::string_view value)
{
	auto key = convert_string(std::string(name));
	auto val = convert_string(std::string(value));
	if (!key || !val) {
		throw std::runtime_error("libenvpp: set_environment_variable failed in string conversion");
	}
	[[maybe_unused]] const auto env_var_was_set = SetEnvironmentVariableW(key->c_str(), val->c_str());
	LIBENVPP_CHECK(env_var_was_set);
}

void delete_environment_variable(const std::string_view name)
{
	auto key = convert_string(std::string(name));
	if (!key) {
		throw std::runtime_error("libenvpp: delete_environment_variable failed in string conversion");
	}
	[[maybe_unused]] const auto env_var_was_deleted = SetEnvironmentVariableW(key->c_str(), nullptr);
	LIBENVPP_CHECK(env_var_was_deleted);
}

} // namespace env::detail

#endif
