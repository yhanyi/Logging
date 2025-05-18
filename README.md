# Logging

An attempt at a high-performance, header-only logging library for C++17+.
Designed with multithreading support, minimal overhead through zero-cost abstractions, and a singleton service pattern.

## Motivation

I grew tired of spamming my C++ code with `std::cout << (...) << '\n'` and the outputs lacked colour (not aesthetically pleasing).

This logger is built to be a tool I would want to use: (relatively) simple logic, colourful, easy to understand and efficient.

## Quick Start

1. Include the header.

```cpp
#include "logger.hpp"
```

2. Basic usage.

```cpp
#include "logger.hpp"
int main() {
    // Configure (optional)
    logging::set_level(logging::Level::DEBUG);
    logging::set_include_location(true);

    // Start logging!
    LOG_INFO("Application started.");
    LOG_DEBUG("Debug value: ", 69));
    LOG_ERROR("Error code: ", -1, " message: ", "File not found!");
    return 0;
}
```

3. Compile.

```bash
g++ -std=c++17 -pthread main.cpp -o main
```

## Log Levels

The logger supports 6 log levels in order of severity:

```cpp
LOG_TRACE("Detailed trace information");     // Level::TRACE
LOG_DEBUG("Debug information");              // Level::DEBUG
LOG_INFO("General information");             // Level::INFO
LOG_WARN("Warning message");                 // Level::WARN
LOG_ERROR("Error occurred");                 // Level::ERROR
LOG_FATAL("Fatal error");                    // Level::FATAL
```

## Configuration

Configure the logger at runtime:

```cpp
// Set minimum log level (filters out lower levels)
logging::set_level(logging::Level::INFO);

// Include file and line information
logging::set_include_location(true);  // Default: false

// Include thread ID in logs
logging::set_include_thread_id(true); // Default: true

// Enable/disable ANSI colors
logging::set_use_colours(true);        // Default: true

// Check current level
auto level = logging::get_level();
```

## Variadic Logging

Log multiple values in a single call:

```cpp
std::string user = "alice";
int id = 12345;
double score = 98.7;

LOG_INFO("User: ", user, " ID: ", id, " Score: ", score);
// Output: [2025-01-01 00:00:00.000] [12345] [INFO] User: alice ID: 12345 Score: 98.7
```

## Thread Safe

The logger is thread-safe out of the box. See `examples.cpp`.

```cpp
#include <thread>

void worker_thread(int id) {
    for (int i = 0; i < 1000; ++i) {
        LOG_INFO("Thread ", id, " processing item ", i);
    }
}

int main() {
    logging::set_include_thread_id(true);

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back(worker_thread, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

## Output Format

Default log format:

```text
[TIMESTAMP] [THREAD_ID] [LEVEL] MESSAGE [LOCATION]
```

Example output:

```text
[2024-01-15 10:30:45.123] [12345] [INFO] Application started
[2024-01-15 10:30:45.124] [12345] [DEBUG] Processing user: alice (main.cpp:42)
[2024-01-15 10:30:45.125] [67890] [WARN] Thread 2 detected anomaly
[2024-01-15 10:30:45.126] [12345] [ERROR] Connection failed: timeout
```

## Note

Thanks for checking out the project! :)
