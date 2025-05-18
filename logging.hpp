#pragma once

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string_view>
#include <thread>

namespace logging {

enum class Level : int {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4,
  FATAL = 5,
};

namespace detail {
// Thread-safe singleton
class State {
public:
  static State &instance() {
    static State instance;
    return instance;
  }

  std::atomic<Level> current_level{Level::INFO};
  std::atomic<bool> include_location{false};
  std::atomic<bool> include_thread_id{true};
  std::atomic<bool> use_colours{true};

private:
  State() = default;
};

// ANSI colours
constexpr std::string_view get_colour_code(Level level) {
  switch (level) {
  case Level::TRACE:
    return "90"; // Dark gray
  case Level::DEBUG:
    return "36"; // Cyan
  case Level::INFO:
    return "37"; // White
  case Level::WARN:
    return "33"; // Yellow
  case Level::ERROR:
    return "31"; // Red
  case Level::FATAL:
    return "41;97"; // Red background, white text
  default:
    return "37";
  }
}

constexpr std::string_view get_level_name(Level level) {
  switch (level) {
  case Level::TRACE:
    return "TRACE";
  case Level::DEBUG:
    return "DEBUG";
  case Level::INFO:
    return "INFO";
  case Level::WARN:
    return "WARN";
  case Level::ERROR:
    return "ERROR";
  case Level::FATAL:
    return "FATAL";
  default:
    return "UNKNOWN";
  }
}

// Thread-local buffer for efficient formatting
class ThreadLocalBuffer {
public:
  static ThreadLocalBuffer &instance() {
    thread_local ThreadLocalBuffer buffer;
    return buffer;
  }

  std::stringstream &get() {
    stream_.clear();
    stream_.str("");
  }

private:
  std::stringstream stream_;
};

// Format timestamp
std::string format_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
  return ss.str();
}

std::string get_thread_id() {
  std::stringstream ss;
  ss << std::this_thread::get_id();
  return ss.str();
}

// Core logging function
template <typename... Args>
void log_impl(Level level, std::string_view file, int line, Args &&...args) {
  static std::mutex output_mutex;
  const auto &state = State::instance();
  // Early exit if log level is disabled
  if (level < state.current_level.load(std::memory_order_relaxed)) {
    return;
  }
  // Format message in thread-local buffer first
  auto &buffer = ThreadLocalBuffer::instance().get();
  (buffer << ... << args);
  std::string message = buffer.str();

  // Construct the log line
  std::stringstream log_line;

  // Colours
  bool use_colours = state.use_colours.load(std::memory_order_relaxed);
  if (use_colours) {
    log_line << "\033[" << get_colour_code(level) << "m";
  }

  // Timestamp
  log_line << "[" << format_timestamp() << "]";

  // Thread ID
  if (state.include_thread_id.load(std::memory_order_relaxed)) {
    log_line << " [" << get_thread_id() << "]";
  }

  // Log level
  log_line << " [" << get_level_name(level) << "] ";

  // Message
  log_line << message;

  // Location info
  if (state.include_location.load(std::memory_order_relaxed)) {
    log_line << " (" << file << ":" << line << ")";
  }

  // Reset colour
  if (use_colours) {
    log_line << "\033[0m";
  }

  log_line << "\n";

  // Thread-safe output to stderr
  {
    std::lock_guard<std::mutex> lock(output_mutex);
    std::cerr << log_line.str();
    std::cerr.flush();
  }
}

inline bool is_level_enabled(Level level) {
  return level >=
         State::instance().current_level.load(std::memory_order_relaxed);
}

} // namespace detail

// Public confuiguration functions
inline void set_level(Level level) {
  detail::State::instance().current_level.store(level,
                                                std::memory_order_relaxed);
}

inline void set_include_location(bool enable) {
  detail::State::instance().include_location.store(enable,
                                                   std::memory_order_relaxed);
}

inline void set_include_thread_id(bool enable) {
  detail::State::instance().include_thread_id.store(enable,
                                                    std::memory_order_relaxed);
}

inline void set_use_colours(bool enable) {
  detail::State::instance().use_colours.store(enable,
                                              std::memory_order_relaxed);
}

inline Level get_level() {
  return detail::State::instance().current_level.load(
      std::memory_order_relaxed);
}

// Conditional logging macros that avoid argument evaluation when disabled

#define LOG_TRACE(...)                                                         \
  do {                                                                         \
    if (::logging::detail::is_level_enabled(::logging::Level::TRACE)) {        \
      ::logging::detail::log_impl(::logging::Level::TRACE, __FILE__, __LINE__, \
                                  __VA_ARGS__);                                \
    }                                                                          \
  } while (0)

#define LOG_DEBUG(...)                                                         \
  do {                                                                         \
    if (::logging::detail::is_level_enabled(::logging::Level::DEBUG)) {        \
      ::logging::detail::log_impl(::logging::Level::DEBUG, __FILE__, __LINE__, \
                                  __VA_ARGS__);                                \
    }                                                                          \
  } while (0)

#define LOG_INFO(...)                                                          \
  do {                                                                         \
    if (::logging::detail::is_level_enabled(::logging::Level::INFO)) {         \
      ::logging::detail::log_impl(::logging::Level::INFO, __FILE__, __LINE__,  \
                                  __VA_ARGS__);                                \
    }                                                                          \
  } while (0)

#define LOG_WARN(...)                                                          \
  do {                                                                         \
    if (::logging::detail::is_level_enabled(::logging::Level::WARN)) {         \
      ::logging::detail::log_impl(::logging::Level::WARN, __FILE__, __LINE__,  \
                                  __VA_ARGS__);                                \
    }                                                                          \
  } while (0)

#define LOG_ERROR(...)                                                         \
  do {                                                                         \
    if (::logging::detail::is_level_enabled(::logging::Level::ERROR)) {        \
      ::logging::detail::log_impl(::logging::Level::ERROR, __FILE__, __LINE__, \
                                  __VA_ARGS__);                                \
    }                                                                          \
  } while (0)

#define LOG_FATAL(...)                                                         \
  do {                                                                         \
    if (::logging::detail::is_level_enabled(::logging::Level::FATAL)) {        \
      ::logging::detail::log_impl(::logging::Level::FATAL, __FILE__, __LINE__, \
                                  __VA_ARGS__);                                \
    }                                                                          \
  } while (0)

} // namespace logging
