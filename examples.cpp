#include "logger.hpp"
#include <chrono>
#include <thread>
#include <vector>

// Test 1: Basic multithreading stress test
void test_1() {
  logging::set_level(logging::Level::TRACE);

  LOG_INFO("=== Basic Multithreading Test ===");

  constexpr int num_threads = 8;
  constexpr int logs_per_thread = 1000;

  auto worker = [](int thread_id) {
    for (int i = 0; i < logs_per_thread; ++i) {
      LOG_TRACE("Thread ", thread_id, " iteration ", i);
      LOG_DEBUG("Thread ", thread_id, " processing item ", i);

      if (i % 100 == 0) {
        LOG_INFO("Thread ", thread_id, " checkpoint at ", i);
      }

      if (i % 500 == 0) {
        LOG_WARN("Thread ", thread_id, " halfway point reached");
      }

      // Simulate some work
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    LOG_INFO("Thread ", thread_id, " completed all iterations");
  };

  std::vector<std::thread> threads;
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker, i);
  }

  for (auto &t : threads) {
    t.join();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  LOG_INFO("Basic test completed. Duration: ", duration.count(), "ms");
  LOG_INFO("Total logs: ", num_threads * logs_per_thread * 4, " across ",
           num_threads, " threads");
}

int main() {
  // Configure logger
  logging::set_level(logging::Level::TRACE);
  logging::set_include_thread_id(true);
  logging::set_include_location(true);
  logging::set_use_colours(true);

  LOG_INFO("Starting comprehensive multithreading tests...");

  try {
    test_1();
    std::this_thread::sleep_for(std::chrono::seconds(1));

  } catch (const std::exception &e) {
    LOG_FATAL("Test suite failed with exception: ", e.what());
    return 1;
  }

  LOG_INFO("All multithreading tests completed successfully!");
  return 0;
}
