#include "logger.hpp"
#include <chrono>
#include <future>
#include <random>
#include <thread>
#include <vector>

// Test 1: Basic multithreading stress test
void test1() {
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

// Test 2: Producer consumer test
void test2() {
  LOG_INFO("=== Producer-Consumer Test ===");

  constexpr int num_producers = 4;
  constexpr int num_consumers = 3;
  constexpr int items_per_producer = 500;

  std::queue<int> work_queue;
  std::mutex queue_mutex;
  std::condition_variable cv;
  std::atomic<bool> producers_done{false};
  std::atomic<int> items_produced{0};
  std::atomic<int> items_consumed{0};

  // Producer function
  auto producer = [&](int producer_id) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);

    LOG_INFO("Producer ", producer_id, " starting");

    for (int i = 0; i < items_per_producer; ++i) {
      int item = dis(gen);

      {
        std::lock_guard<std::mutex> lock(queue_mutex);
        work_queue.push(item);
        items_produced.fetch_add(1);
      }
      cv.notify_one();

      LOG_TRACE("Producer ", producer_id, " produced item ", item, " (", i + 1,
                "/", items_per_producer, ")");

      if ((i + 1) % 100 == 0) {
        LOG_DEBUG("Producer ", producer_id, " progress: ", i + 1, "/",
                  items_per_producer);
      }

      // Random delay
      std::this_thread::sleep_for(std::chrono::microseconds(dis(gen) % 100));
    }

    LOG_INFO("Producer ", producer_id, " finished");
  };

  // Consumer function
  auto consumer = [&](int consumer_id) {
    LOG_INFO("Consumer ", consumer_id, " starting");
    int consumed_count = 0;

    while (true) {
      std::unique_lock<std::mutex> lock(queue_mutex);
      cv.wait(lock,
              [&] { return !work_queue.empty() || producers_done.load(); });

      if (work_queue.empty() && producers_done.load()) {
        break;
      }

      if (!work_queue.empty()) {
        int item = work_queue.front();
        work_queue.pop();
        lock.unlock();

        // Simulate processing
        std::this_thread::sleep_for(std::chrono::microseconds(item % 200));

        consumed_count++;
        items_consumed.fetch_add(1);

        LOG_TRACE("Consumer ", consumer_id, " processed item ", item,
                  " (total: ", consumed_count, ")");

        if (consumed_count % 50 == 0) {
          LOG_DEBUG("Consumer ", consumer_id, " processed ", consumed_count,
                    " items so far");
        }
      }
    }

    LOG_INFO("Consumer ", consumer_id, " finished after processing ",
             consumed_count, " items");
  };

  // Start consumers first
  std::vector<std::thread> threads;
  for (int i = 0; i < num_consumers; ++i) {
    threads.emplace_back(consumer, i);
  }

  // Start producers
  for (int i = 0; i < num_producers; ++i) {
    threads.emplace_back(producer, i);
  }

  // Wait for producers to finish
  auto producers_start = threads.end() - num_producers;
  for (auto it = producers_start; it != threads.end(); ++it) {
    it->join();
  }

  producers_done.store(true);
  cv.notify_all();

  // Wait for consumers to finish
  for (int i = 0; i < num_consumers; ++i) {
    threads[i].join();
  }

  LOG_INFO("Producer-Consumer test completed");
  LOG_INFO("Total produced: ", items_produced.load(),
           ", Total consumed: ", items_consumed.load());
}

// Test 3: Parallel task processing test
void test3() {
  LOG_INFO("=== Parallel Task Processing Test ===");

  constexpr int num_tasks = 20;

  // Simulate different types of tasks
  auto cpu_intensive_task = [](int task_id) -> double {
    LOG_DEBUG("Starting CPU-intensive task ", task_id);

    double result = 0.0;
    for (int i = 0; i < 1000000; ++i) {
      result += std::sin(i) * std::cos(i);
    }

    LOG_DEBUG("Completed CPU-intensive task ", task_id, " with result ",
              result);
    return result;
  };

  auto io_simulation_task = [](int task_id) -> std::string {
    LOG_DEBUG("Starting I/O simulation task ", task_id);

    // Simulate I/O delay
    std::this_thread::sleep_for(
        std::chrono::milliseconds(100 + (task_id % 50)));

    std::stringstream result;
    result << "data_from_task_" << task_id;

    LOG_DEBUG("Completed I/O simulation task ", task_id);
    return result.str();
  };

  std::vector<std::future<double>> cpu_futures;
  std::vector<std::future<std::string>> io_futures;

  // Launch CPU-intensive tasks
  for (int i = 0; i < num_tasks / 2; ++i) {
    cpu_futures.push_back(
        std::async(std::launch::async, cpu_intensive_task, i));
    LOG_TRACE("Launched CPU task ", i);
  }

  // Launch I/O simulation tasks
  for (int i = num_tasks / 2; i < num_tasks; ++i) {
    io_futures.push_back(std::async(std::launch::async, io_simulation_task, i));
    LOG_TRACE("Launched I/O task ", i);
  }

  // Collect results
  double total_cpu_result = 0.0;
  for (int i = 0; i < cpu_futures.size(); ++i) {
    double result = cpu_futures[i].get();
    total_cpu_result += result;
    LOG_INFO("CPU task ", i, " completed with result: ", result);
  }

  std::vector<std::string> io_results;
  for (int i = 0; i < io_futures.size(); ++i) {
    std::string result = io_futures[i].get();
    io_results.push_back(result);
    LOG_INFO("I/O task ", i + num_tasks / 2,
             " completed with result: ", result);
  }

  LOG_INFO("All parallel tasks completed");
  LOG_INFO("Total CPU result: ", total_cpu_result);
  LOG_INFO("Total I/O results collected: ", io_results.size());
}

// Test 4: Thread pool simulation with error handling test
void test4() {
  LOG_INFO("=== Thread Pool Simulation Test ===");

  constexpr int pool_size = 6;
  constexpr int total_jobs = 50;

  std::queue<std::function<void()>> job_queue;
  std::mutex queue_mutex;
  std::condition_variable cv;
  std::atomic<bool> shutdown{false};
  std::atomic<int> completed_jobs{0};

  // Worker thread function
  auto worker = [&](int worker_id) {
    LOG_INFO("Worker ", worker_id, " started");

    while (true) {
      std::function<void()> job;

      {
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv.wait(lock, [&] { return !job_queue.empty() || shutdown.load(); });

        if (shutdown.load() && job_queue.empty()) {
          break;
        }

        if (!job_queue.empty()) {
          job = std::move(job_queue.front());
          job_queue.pop();
        }
      }

      if (job) {
        try {
          LOG_TRACE("Worker ", worker_id, " executing job");
          job();
          completed_jobs.fetch_add(1);
          LOG_TRACE("Worker ", worker_id, " completed job successfully");
        } catch (const std::exception &e) {
          LOG_ERROR("Worker ", worker_id,
                    " job failed with exception: ", e.what());
        } catch (...) {
          LOG_ERROR("Worker ", worker_id, " job failed with unknown exception");
        }
      }
    }

    LOG_INFO("Worker ", worker_id, " shutting down");
  };

  // Start worker threads
  std::vector<std::thread> workers;
  for (int i = 0; i < pool_size; ++i) {
    workers.emplace_back(worker, i);
  }

  // Generate jobs
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 100);

  for (int i = 0; i < total_jobs; ++i) {
    auto job = [i, &dis, &gen]() {
      int work_time = dis(gen);
      LOG_DEBUG("Executing job ", i, " (estimated ", work_time, "ms)");

      // Simulate work
      std::this_thread::sleep_for(std::chrono::milliseconds(work_time));

      // Simulate occasional failures
      if (work_time > 90) {
        throw std::runtime_error("Simulated job failure");
      }

      LOG_DEBUG("Job ", i, " completed successfully");
    };

    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      job_queue.push(std::move(job));
    }
    cv.notify_one();

    LOG_TRACE("Submitted job ", i);

    // Small delay between job submissions
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  LOG_INFO("All jobs submitted, waiting for completion");

  // Wait for all jobs to complete
  while (completed_jobs.load() < total_jobs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    LOG_DEBUG("Progress: ", completed_jobs.load(), "/", total_jobs,
              " jobs completed");
  }

  // Shutdown workers
  shutdown.store(true);
  cv.notify_all();

  for (auto &worker : workers) {
    worker.join();
  }

  LOG_INFO("Thread pool simulation completed");
  LOG_INFO("Final stats: ", completed_jobs.load(), "/", total_jobs,
           " jobs completed successfully");
}

// Test 5: Rapid logging test
void test5() {
  LOG_INFO("=== Rapid Logging Stress Test ===");

  constexpr int num_threads = 12;
  constexpr int logs_per_thread = 10000;

  std::atomic<int> total_logs{0};

  auto rapid_logger = [&](int thread_id) {
    for (int i = 0; i < logs_per_thread; ++i) {
      // Mix different log levels
      switch (i % 6) {
      case 0:
        LOG_TRACE("Trace from thread ", thread_id, " iteration ", i);
        break;
      case 1:
        LOG_DEBUG("Debug from thread ", thread_id, " iteration ", i);
        break;
      case 2:
        LOG_INFO("Info from thread ", thread_id, " iteration ", i);
        break;
      case 3:
        LOG_WARN("Warning from thread ", thread_id, " iteration ", i);
        break;
      case 4:
        LOG_ERROR("Error from thread ", thread_id, " iteration ", i);
        break;
      case 5:
        LOG_FATAL("Fatal from thread ", thread_id, " iteration ", i);
        break;
      }
      total_logs.fetch_add(1);
    }
  };

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(rapid_logger, i);
  }

  for (auto &t : threads) {
    t.join();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  LOG_INFO("Stress test completed");
  LOG_INFO("Total logs: ", total_logs.load());
  LOG_INFO("Duration: ", duration.count(), "ms");
  LOG_INFO("Logs per second: ", (total_logs.load() * 1000) / duration.count());
}

int main() {
  // Configure logger
  logging::set_level(logging::Level::TRACE);
  logging::set_include_thread_id(true);
  logging::set_include_location(true);
  logging::set_use_colours(true);

  LOG_INFO("Starting comprehensive multithreading tests...");

  try {
    test1(); // Test 1: Basic multithreading stress test
    test2(); // Test 2: Producer consumer test
    test3(); // Test 3: Parallel task processing test
    test4(); // Test 4: Thread pool simulation with error handling test
    test5(); // Test 5: Rapid logging test

    std::this_thread::sleep_for(std::chrono::seconds(1));

  } catch (const std::exception &e) {
    LOG_FATAL("Test suite failed with exception: ", e.what());
    return 1;
  }

  LOG_INFO("All multithreading tests completed successfully!");
  return 0;
}
