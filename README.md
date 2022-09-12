# libenvpp - Modern C++ Library for Handling Environment Variables

## Usage

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
const auto num_threads_id = pre.register_variable<unsigned int>("NUM_THREADS");
```

After having registered all variables with the corresponding prefix it can be parsed and evaluated, which will retrieve the values from the environment and parse them into the type specified when registering:

```cpp
const auto parsed_and_validated_pre = pre.parse_and_validate();
```

This will invalidate the prefix `pre` which must not be used after this point, and can be safely destroyed.

If anything went wrong this can be queried through the `parsed_and_validated_pre`:

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
    const auto num_threads = parsed_and_validated_pre.get_or(num_threads_id, 1);
}
```

### Simple Example

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
    const auto num_threads_id = pre.register_variable<unsigned int>("NUM_THREADS");

    const auto parsed_and_validated_pre = pre.parse_and_validate();

    if (parsed_and_validated_pre.ok()) {
        const auto log_path = parsed_and_validated_pre.get_or(log_path_id, "/default/log/path");
        const auto num_threads = parsed_and_validated_pre.get_or(num_threads_id, 1);

        std::cout << "Log path   : " << log_path << std::endl;
        std::cout << "Num threads: " << num_threads << std::endl;
    } else {
        std::cout << parsed_and_validated_pre.warning_message() << std::endl;
        std::cout << parsed_and_validated_pre.error_message() << std::endl;
    }

    return EXIT_SUCCESS;
}
```
