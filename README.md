# libenvpp - Modern C++ Library for Handling Environment Variables

## Features

- Platform independent
- Type safe parsing of environment variables
- Support for required and optional environment variables, with specifiable default value
- Automatic parsing of built-in types
- User-defined types parsable with user-defined parser
- Optional user-defined validation
- Convenience range/option environment variable type
- Parsing/validating possible per type, or per environment variable
- Typo detection based on edit-distance
- Unused environment variable detection

## Usage

### Simple Example

To use the library the first step would be to include the header:

```cpp
#include <libenvpp.hpp>
```

The library is built around the idea of a `prefix`. The assumption is that all relevant environment variables used by a program share the same prefix, for example with the prefix `MYPROG`:

```text
MYPROG_LOG_FILE_PATH="/path/to/log"
MYPROG_NUM_THREADS=4
```

This would be expressed as:

```cpp
auto pre = env::prefix("MYPROG");

const auto log_path_id = pre.register_variable<std::filesystem::path>("LOG_FILE_PATH");
const auto num_threads_id = pre.register_required_variable<unsigned int>("NUM_THREADS");
```

Here `MYPROG_LOG_FILE_PATH` is registered as an optional environment variable, which will not cause an error or warning when it is not specified in the environment. Whereas `MYPROG_NUM_THREADS` is registered as a required environment variable, which will cause an error if it is not found in the environment.

After having registered all variables with the corresponding prefix it can be parsed and validated, which will retrieve the values from the environment and parse them into the type specified when registering:

```cpp
const auto parsed_and_validated_pre = pre.parse_and_validate();
```

This will invalidate the prefix `pre` which must not be used after this point, and can be safely destroyed.

If anything went wrong, like the required variable not being found, or the variable not being parsable, this can be queried through the `parsed_and_validated_pre`:

```cpp
if (!parsed_and_validated_pre.ok()) {
    std::cout << parsed_and_validated_pre.warning_message() << std::endl;
    std::cout << parsed_and_validated_pre.error_message() << std::endl;
}
```

Otherwise the parsed values can be retrieved from the `parsed_and_validated_pre`:

```cpp
if (parsed_and_validated_pre.ok()) {
    const auto log_path = parsed_and_validated_pre.get_or(log_path_id, "/default/log/path");
    const auto num_threads = parsed_and_validated_pre.get(num_threads_id);
}
```

Optional variables can be given a default value when getting them with `get_or`, which will return the default value if (and only if) the variable was not found in the environment. Parsing or validation errors of optional variables are also reported as errors. Additionally, optional variables can also be retrieved with `get` which will return a `std::optional` which is empty if the variable was not found in the environment.

Required variables can only be gotten with `get`, as they are required and therefore need no default value, which will return the type directly. If required variables were not found in the environment this will be reported as an error.

#### Simple Example - Code

Putting everything together:

```cpp
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <libenvpp.hpp>

int main()
{
    auto pre = env::prefix("MYPROG");

    const auto log_path_id = pre.register_variable<std::filesystem::path>("LOG_FILE_PATH");
    const auto num_threads_id = pre.register_required_variable<unsigned int>("NUM_THREADS");

    const auto parsed_and_validated_pre = pre.parse_and_validate();

    if (parsed_and_validated_pre.ok()) {
        const auto log_path = parsed_and_validated_pre.get_or(log_path_id, "/default/log/path");
        const auto num_threads = parsed_and_validated_pre.get(num_threads_id);

        std::cout << "Log path   : " << log_path << std::endl;
        std::cout << "Num threads: " << num_threads << std::endl;
    } else {
        std::cout << parsed_and_validated_pre.warning_message() << std::endl;
        std::cout << parsed_and_validated_pre.error_message() << std::endl;
    }

    return EXIT_SUCCESS;
}
```

### Custom Type Parser

To provide a parser for a user-defined type it is necessary to specialize the template struct `default_parser` for the specific type that should be parsed and provide a call operator that takes a `std::string_view` and returns the parsed type, for example:

```cpp
struct program_data {
    int number;
    float percent;
};

namespace env {
template <>
struct default_parser<program_data> {
    program_data operator()(const std::string_view str) const
    {
        const auto split_str = split(str, ',');
        if (split_str.size() != 2) {
            // Report an error if the input does not have the expected format
            throw parser_error{"Expected 2 comma delimited values"};
        }

        auto parsed_data = program_data{};

        // Delegate parsing of primitive types to the default_parser
        parsed_data.number = default_parser<int>{}(split_str[0]);
        parsed_data.percent = default_parser<float>{}(split_str[1]);

        return parsed_data;
    }
};
} // namespace env
```

_Note:_ The `default_parser` already supports primitive types (and everything that can be constructed from string), so parsing should be delegated to the existing implementation whenever possible.

#### Custom Type Parser - Code

For the entire code see [examples/libenvpp_custom_parser_example.cpp](examples/libenvpp_custom_parser_example.cpp).

### Custom Type Validator

Similarly to the `default_parser` there is also a `default_validator` which can be specialized for any type that should be validated. The non-specialized implementation of `default_validator` does nothing, as everything that can be parsed is considered valid by default. When specializing `default_validator` for a type the `struct` must contain a call operator that takes the parsed type and returns `void`. Validation errors should be reported by throwing a `validation_error`:

```cpp
namespace env {
template <>
struct default_validator<std::filesystem::path> {
    void operator()(const std::filesystem::path& path) const
    {
        if (!std::filesystem::exists(path)) {
            throw validation_error{path.string() + " path does not exist"};
        }
    }
};
} // namespace env
```

The validator example above shows a validator for the type `std::filesystem::path` which requires that environment variables of that type must exist on the filesystem, otherwise the specified environment variable is considered invalid, even if the string was a valid path.

#### Custom Type Validator - Code

For the full example see [examples/libenvpp_custom_validator_example.cpp](examples/libenvpp_custom_validator_example.cpp).
