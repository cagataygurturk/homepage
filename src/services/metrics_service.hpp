#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace homepage::services {

struct SystemMetrics {
  double cpu_usage = 0.0;
  std::optional<double> cpu_temperature;
  std::optional<double> fan_speed;
  std::string node_name;
};

// Samples system metrics on a single background thread at a fixed interval,
// publishes the latest snapshot to any number of concurrent readers and
// notifies subscribers after every sample.
class MetricsService final {
 public:
  using Listener = std::function<void(const SystemMetrics&)>;
  using SubscriptionId = std::size_t;

  explicit MetricsService(
      std::chrono::milliseconds interval = std::chrono::seconds(2));
  ~MetricsService();

  MetricsService(const MetricsService&) = delete;
  MetricsService& operator=(const MetricsService&) = delete;
  MetricsService(MetricsService&&) = delete;
  MetricsService& operator=(MetricsService&&) = delete;

  // Starts the sampler thread. Calling it again while running is a no-op.
  void start();

  // Stops and joins the sampler thread. Safe to call more than once.
  void stop();

  // Returns a copy of the most recent sample.
  [[nodiscard]] SystemMetrics latest() const;

  // Registers a listener that is invoked on the sampler thread after every
  // sample. Listeners must be quick and must not call subscribe/unsubscribe.
  SubscriptionId subscribe(Listener listener);

  // Removes a listener. Returns once no invocation of it is in progress.
  void unsubscribe(SubscriptionId id);

  [[nodiscard]] std::string to_json(const SystemMetrics& metrics) const;

 private:
  void run();
  [[nodiscard]] SystemMetrics sample();
  void publish(const SystemMetrics& metrics);

  // Platform-specific readers, called from the sampler thread only.
  double get_cpu_usage();
  std::optional<double> get_cpu_temperature();
  std::optional<double> get_fan_speed();

  std::chrono::milliseconds interval_;
  std::string node_name_;

  // CPU counters from the previous sample; touched by the sampler thread only.
  unsigned long long prev_total_ = 0;
  unsigned long long prev_idle_ = 0;

  mutable std::mutex mutex_;
  std::condition_variable stop_cv_;
  bool stop_requested_ = false;
  SystemMetrics latest_;
  std::thread sampler_;

  std::mutex listeners_mutex_;
  std::vector<std::pair<SubscriptionId, Listener>> listeners_;
  SubscriptionId next_subscription_ = 1;
};

}  // namespace homepage::services
