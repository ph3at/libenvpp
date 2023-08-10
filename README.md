# libenvpp - Modern C++ Library for Handling Environment Variables

This library provides a modern, platform independent, type-safe way of handling environment variables. It offers an easy to use interface for the most common use-cases, and is extensible and customizable to more complex use-cases, if and when required.

- [Features](#features)
- [Usage](#usage)
  - [Simple Example](#simple-example)
    - [Simple Example - Code](#simple-example---code)
    - [Simple Example - Output](#simple-example---output)
  - [Custom Type Parser](#custom-type-parser)
    - [Custom Type Parser - Code](#custom-type-parser---code)
  - [Custom Type Validator](#custom-type-validator)
    - [Custom Type Validator - Code](#custom-type-validator---code)
  - [Custom Variable Parser and Validator](#custom-variable-parser-and-validator)
    - [Custom Variable Parser and Validator - Code](#custom-variable-parser-and-validator---code)
  - [Range Variables](#range-variables)
    - [Range Variables - Code](#range-variables---code)
  - [Option Variables](#option-variables)
    - [Option Variables - Code](#option-variables---code)
  - [Deprecated Variables](#deprecated-variables)
  - [Prefixless Environment Variables](#prefixless-environment-variables)
    - [Prefixless Environment Variables - Code](#prefixless-environment-variables---code)
- [Error Handling](#error-handling)
  - [Help Message](#help-message)
  - [Warnings and Errors](#warnings-and-errors)
    - [`error` Type](#error-type)
    - [Warnings](#warnings)
    - [Errors](#errors)
  - [Warnings and Errors - Code](#warnings-and-errors---code)
- [Testing](#testing)
  - [Global Testing Environment](#global-testing-environment)
    - [Global Testing Environment - Code](#global-testing-environment---code)
  - [Custom Environment](#custom-environment)
    - [Custom Environment - Code](#custom-environment---code)
  - [Set for Testing](#set-for-testing)
    - [Set for Testing - Code](#set-for-testing---code)
- [Installation](#installation)
  - [FetchContent](#fetchcontent)
  - [Submodule](#submodule)
  - [Cmake Options](#cmake-options)

## Features

- Platform independent
- Type-safe parsing of environment variables
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
#include <libenvpp/env.hpp>
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
    std::cout << parsed_and_validated_pre.warning_message();
    std::cout << parsed_and_validated_pre.error_message();
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

#include <libenvpp/env.hpp>

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
        std::cout << parsed_and_validated_pre.warning_message();
        std::cout << parsed_and_validated_pre.error_message();
    }

    return EXIT_SUCCESS;
}
```

#### Simple Example - Output

Running the example without having any environment variable set:

```console
$ ./libenvpp_simple_usage_example
Error  : Environment variable 'MYPROG_NUM_THREADS' not set
```

Running the example with a typo:

```console
$ MYPROG_LOG_FILEPATH=/var/log/file ./libenvpp_simple_usage_example
Warning: Unrecognized environment variable 'MYPROG_LOG_FILEPATH' set, did you mean 'MYPROG_LOG_FILE_PATH'?
Error  : Environment variable 'MYPROG_NUM_THREADS' not set
```

Running the example with only the required variable set:

```console
$ MYPROG_NUM_THREADS=4 ./libenvpp_simple_usage_example
Log path   : "/default/log/path"
Num threads: 4
```

Running the example with both variables set:

```console
$ MYPROG_LOG_FILE_PATH=/var/log/file MYPROG_NUM_THREADS=4 ./libenvpp_simple_usage_example
Log path   : "/var/log/file"
Num threads: 4
```

### Custom Type Parser

To provide a parser for a user-defined type it is necessary to specialize the template struct `env::default_parser` for the specific type that should be parsed and provide a call operator that takes a `std::string_view` and returns the parsed type, for example:

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

Similarly to the `env::default_parser` there is also an `env::default_validator` which can be specialized for any type that should be validated. The non-specialized implementation of `default_validator` does nothing, as everything that can be parsed is considered valid by default. When specializing `default_validator` for a type the `struct` must contain a call operator that takes the parsed type and returns `void`. Validation errors should be reported by throwing an `env::validation_error`:

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

### Custom Variable Parser and Validator

If parsing/validator should be done in a specific way for a specific variable only, and not for every variable of the same type, it is possible to specify a parser and validator function when registering a variable:

```cpp
std::filesystem::path path_parser_and_validator(const std::string_view str)
{
    const auto log_path = std::filesystem::path(str);

    if (!std::filesystem::exists(log_path)) {
        if (!std::filesystem::create_directory(log_path)) {
            throw env::validation_error{"Unable to create log directory"};
        }
    } else if (!std::filesystem::is_directory(log_path)) {
        throw env::validation_error{"Log path is not a directory"};
    }

    return log_path;
}

int main()
{
    auto pre = env::prefix("CUSTOM_PARSER_AND_VALIDATOR");

    const auto path_id = pre.register_required_variable<std::filesystem::path>("LOG_PATH", path_parser_and_validator);

    /*...*/
}
```

For example, the parser and validator function above will make sure that the log path specified by the environment variable points to a directory, and will even create it if it doesn't exist. Errors are reported by throwing `env::parser_error` or `env::validation_error`.

_Note:_ When specifying a custom parser and validator function, the `default_parser` and `default_validator` for the given type are not invoked automatically, however delegating to them is still possible and should be done if appropriate.

_Note:_ The parser and validator function passed to `register_[required]_variable` can be any callable type, be it function, lambda, or functor.

#### Custom Variable Parser and Validator - Code

For the full example see [examples/libenvpp_custom_parser_and_validator_example.cpp](examples/libenvpp_custom_parser_and_validator_example.cpp).

### Range Variables

Because it is a frequent use-case that a value must be within a given range, environment variables can be registered with `register_range` which additionally takes a minimum and maximum value, and validates that the parsed value is within the given range. The minimum and maximum values are both **inclusive**. For example:

```cpp
auto pre = env::prefix("RANGE");
const auto num_threads_id = pre.register_range<unsigned int>("NUM_THREADS", 1, std::thread::hardware_concurrency());
const auto parsed_and_validated_pre = pre.parse_and_validate();

if (parsed_and_validated_pre.ok()) {
    const auto num_threads = parsed_and_validated_pre.get_or(num_threads_id, std::thread::hardware_concurrency());
    std::cout << "Number of threads: " << num_threads << std::endl;
} else {
    std::cout << parsed_and_validated_pre.warning_message();
    std::cout << parsed_and_validated_pre.error_message();
}
```

This will enforce that the specified number of threads is between 1 and the maximum that the system this is run on supports, inclusive.

_Note:_ Range variables, just like normal variables, can be optional or required. And the type is parsed with `default_parser` and validated with `default_validator`. It is not possible, however, to specify a custom parser and validator function for a range, if this is desired, a normal variable should be used and the range check done manually as part of the validation.

_Note:_ When using `get_or` to set a default value for an optional range, the range requirement is **not** enforced on the default. This can be used for special case handling if desired.

#### Range Variables - Code

For the complete code see [examples/libenvpp_range_example.cpp](examples/libenvpp_range_example.cpp).

### Option Variables

Another frequent use-case is that a value is one of a given set of options. For this an environment variable can be registered with `register_[required]_option`, which takes a list of valid options, against which the value is checked. For example:

```cpp
enum class option {
    first_choice,
    second_choice,
    third_choice,
    default_choice,
};

int main()
{
    auto pre = env::prefix("OPTION");

    const auto option_id =
        pre.register_option<option>("CHOICE", {option::first_choice, option::second_choice, option::third_choice});

    const auto parsed_and_validated_pre = pre.parse_and_validate();

    if (parsed_and_validated_pre.ok()) {
        const auto opt = parsed_and_validated_pre.get_or(option_id, option::default_choice);
    }
}
```

This registers an `enum class` option, where only a subset of all possible values is considered valid, so that `option::default_choice` can be used as the value if the variable is not set.

_Note:_ The list of options provided when registering must not be empty, and must not contain duplicates.

_Note:_ As with range variables, the default value given with `get_or` is not enforced to be within the list of options given when registering the option variable.

_Note:_ Since C++ does not provide any way to automatically parse `enum class` types from string, the example above additionally requires a specialized `default_parser` for the `enum class` type.

_Note:_ Options are mostly intended to be used with `enum class` types, but this is in no way a requirement. Any type can be used as an option, and `enum class` types can also just be normal environment variables.

#### Option Variables - Code

For the full code, including the parser for the enum class, see [examples/libenvpp_option_example.cpp](examples/libenvpp_option_example.cpp).

### Deprecated Variables

Sometimes, new releases of your software will deprecate environment variables. To provide users with helpful messages in this case, you can use the 'register_deprecated' feature.

```cpp
#include <libenvpp/env.hpp>

int main()
{
    auto pre = env::prefix("APP");
    pre.register_deprecated("FEATURE", "The option 'APP_FEATURE' has been deprecated since version x.y. Please use SOMETHING instead.");
}
```

### Prefixless Environment Variables

Even though it is recommended to namespace environment variables with a prefix, and use the prefix mechanism of this library to parse those variables, sometimes it might be necessary to parse environment variables that don't have a prefix. To this end, this library also provides a mechanism for that:

```cpp
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <libenvpp/env.hpp>

int main()
{
    const auto log_path = env::get_or<std::filesystem::path>("LOG_FILE_PATH", "/default/log/path");
    const auto num_threads = env::get<unsigned int>("NUM_THREADS");

    if (num_threads.has_value()) {
        std::cout << "Log path   : " << log_path << std::endl;
        std::cout << "Num threads: " << num_threads.value() << std::endl;
    } else {
        std::cout << num_threads.error().what() << std::endl;
    }

    return EXIT_SUCCESS;
}

```

This is analogous to the simple usage example using a prefix. Required variables can be parsed using `env::get`, which either returns the parsed and validated value, or an error.

Optional variables can be parsed and validated with `env::get_or` which will always return a value, either the successfully parsed and validated one from the environment, or the default value.

_Note:_ These functions make use of the same `default_parser`/`default_validator` mechanism as the prefix, and so parsing/validating of user-defined types is also supported.

#### Prefixless Environment Variables - Code

For the code of this example, see [examples/libenvpp_prefixless_get_example.cpp](examples/libenvpp_prefixless_get_example.cpp).

## Error Handling

### Help Message

After having registered all variables with a prefix it is possible to generate a formatted help message for a specific prefix. This can be done by calling `help_message` on either a prefix, or a validated prefix. For example:

```cpp
auto pre = env::prefix("MYPROG");

const auto log_path_id = pre.register_variable<std::filesystem::path>("LOG_FILE_PATH");
const auto num_threads_id = pre.register_required_variable<unsigned int>("NUM_THREADS");

std::cout << pre.help_message();
```

will output:

```text
Prefix 'MYPROG' supports the following 2 environment variable(s):
    'MYPROG_LOG_FILE_PATH' optional
    'MYPROG_NUM_THREADS' required
```

### Warnings and Errors

If anything goes wrong when parsing and validating a prefix this can be queried through the following functions:

| Function            | Return value                                                   |
|---------------------|----------------------------------------------------------------|
| `ok()`              | `bool` indicating whether there are any errors or warnings     |
| `warning_message()` | `std::string` with a formatted message containing all warnings |
| `error_message()`   | `std::string` with a formatted message containing all errors   |
| `warnings()`        | `std::vector<env::error>` containing all warnings              |
| `errors()`          | `std::vector<env::error>` containing all errors                |

#### `error` Type

The `env::error` type is used to give more information on warnings and errors. The type supports:

| Function     | Return value                                                                                                                                |
|--------------|---------------------------------------------------------------------------------------------------------------------------------------------|
| `get_id()`   | `std::size_t` ID of the variable that caused the error/warning. This ID can be compared to the ID returned from the `register_*` functions. |
| `get_name()` | `std::string` containing the name of the variable that caused the warning/error.                                                            |
| `what()`     | `std::string` containing the warning/error message.                                                                                         |

#### Warnings

The following situations will generate a warning:

- An optional variable is not set, but a similar (within edit distance configured for the prefix) unused variable is.
- An unused variable with the same prefix is set.

#### Errors

The following situations will generate an error:

- A variable could not be parsed.
- A variable could not be validated.
- The value of a range variable is outside the specified range.
- The value of an option variable is not a valid option.
- An unexpected exception was thrown while parsing/validating a variable.
- A required variable is not set, but a similar (within edit distance configured for the prefix) unused variable is.
- A required variable is not set.

### Warnings and Errors - Code

For an example of how the warning/error handling functions can be used, see [examples/libenvpp_error_handling_example.cpp](examples/libenvpp_error_handling_example.cpp).

## Testing

There are a few ways that help facilitate (unit) testing.

### Global Testing Environment

Libenvpp provides an internal global testing environment where variables can be added as key/value pairs using `env::scoped_test_environment`:

```cpp
const auto _ = env::scoped_test_environment({
    {"MYPROG_LOG_FILE_PATH", "/dev/null"},
    {"MYPROG_NUM_THREADS", "8"},
});
```

Once the scoped test environment instance goes out of scope, the variables it added to the global test environment are automatically removed.

Any libenvpp function that usually retrieves variables from the system environment will first check if the global test environment contains a value for the requested variable and retrieve that instead. This can be used to facilitate unit testing without having to change the usage of libenvpp.

_Note:_ Any value set in the global testing environment will take precedence over system/custom environment variables.

_Note:_ Interacting with the global testing environment is not thread safe and the user must take care to synchronize any potentially conflicting accesses.

#### Global Testing Environment - Code

A complete example of how to use the global testing environment, see [examples/libenvpp_testing_example.cpp](examples/libenvpp_testing_example.cpp).

### Custom Environment

Another mechanism that can be used for testing is the ability to pass a custom environment to `prefix::parse_and_validate` as the first parameter of type `std::unordered_map<std::string, std::string>`.

Environment variables will then be fetched from there instead of the system environment. Note that the global testing environment will still take precedence over the custom environment and if the variable is not found in the testing environment or the custom environment **no** fallback to the system environment will be performed.

#### Custom Environment - Code

A complete example of how to use a custom environment can be found here: [examples/libenvpp_custom_environment_example.cpp](examples/libenvpp_custom_environment_example.cpp)

### Set for Testing

Additionally it is also possible to bypass parsing and validating completely and directly set the value for a registered variable on a prefix.

```cpp
auto pre = env::prefix("MYPROG");

const auto log_path_id = pre.register_variable<std::filesystem::path>("LOG_FILE_PATH");
const auto num_threads_id = pre.register_required_variable<unsigned int>("NUM_THREADS");

pre.set_for_testing(log_path_id, "/dev/null");
pre.set_for_testing(num_threads_id, 8);
```

This will ignore any value for that variable in the testing/system/custom environment and not perform any parsing or validating and just directly return the set value when using `get`/`get_or` on the parsed and validated prefix.

#### Set for Testing - Code

A complete example of how to use `set_for_testing` can be found here: [examples/libenvpp_set_for_testing_example.cpp](examples/libenvpp_set_for_testing_example.cpp).

## Installation

### FetchContent

Libenvpp can be integrated into a cmake project (`myproject`) using the FetchContent mechanism like this:

`CMakeLists.txt`:

```cmake
set(LIBENVPP_INSTALL ON CACHE BOOL "" FORCE) # If installation is desired.
FetchContent_Declare(libenvpp
    GIT_REPOSITORY https://github.com/ph3at/libenvpp.git
    GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(libenvpp)

# ...

target_link_libraries(myproject PUBLIC
    libenvpp::libenvpp
)
```

`myproject-config.cmake.in` (only required if installation is desired):

```cmake
# ...

include(CMakeFindDependencyMacro)
find_dependency(libenvpp REQUIRED)

# ...
```

### Submodule

The library uses cmake as the build system and can be easily integrated into an existing cmake project by adding it with `add_subdirectory`, ideally used in combination with a git submodule of the library. The build system will detect this use-case and automatically disable the building of examples and tests.

### Cmake Options

| Option              | Default                                                                 | Description                                                                                                                                               |
|---------------------|-------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------|
| `LIBENVPP_EXAMPLES` | `ON` if configured standalone, `OFF` if used as dependency              | Enables building of example programs.                                                                                                                     |
| `LIBENVPP_TESTS`    | `ON` if configured standalone, `OFF` if used as dependency              | Enables building of unit tests.                                                                                                                           |
| `LIBENVPP_CHECKS`   | `ON` in debug builds or when tests are enabled, `OFF` in release builds | Custom assertions that are not tied to `NDEBUG`, and are testable by throwing an exception instead of `std::abort`ing _iff_ `LIBENVPP_TESTS` are enabled. |
| `LIBENVPP_INSTALL`  | `OFF`                                                                   | Adds an install target that can be used to install libenvpp as a library.                                                                                 |
