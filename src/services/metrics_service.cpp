#include "metrics_service.hpp"

#include <json/json.h>

#include <cstdlib>
#include <exception>
#include <utility>

#include "trantor/utils/Logger.h"

namespace homepage::services {

MetricsService::MetricsService(std::chrono::milliseconds interval)
    : interval_(interval) {
  // Node name comes from the Kubernetes Downward API and never changes.
  const char* node_name = std::getenv("NODE_NAME");
  node_name_ = (node_name != nullptr) ? node_name : "unknown";
  latest_.node_name = node_name_;

  // Prime the CPU counters so the first sample covers a full interval.
  get_cpu_usage();
}

MetricsService::~MetricsService() { stop(); }

void MetricsService::start() {
  if (sampler_.joinable()) {
    return;
  }
  {
    const std::lock_guard lock(mutex_);
    stop_requested_ = false;
  }
  sampler_ = std::thread([this] { run(); });
}

void MetricsService::stop() {
  {
    const std::lock_guard lock(mutex_);
    stop_requested_ = true;
  }
  stop_cv_.notify_all();
  if (sampler_.joinable()) {
    sampler_.join();
  }
}

SystemMetrics MetricsService::latest() const {
  const std::lock_guard lock(mutex_);
  return latest_;
}

void MetricsService::run() {
  std::unique_lock lock(mutex_);
  // Sample once right away so early connections see real values, then keep
  // sampling every interval. wait_for returns true only when a stop was
  // requested; a timeout means it is time for the next sample.
  do {
    lock.unlock();
    try {
      SystemMetrics metrics = sample();
      lock.lock();
      latest_ = std::move(metrics);
    } catch (const std::exception& e) {
      LOG_ERROR << "Metrics sampling failed: " << e.what();
      lock.lock();
    }
  } while (
      !stop_cv_.wait_for(lock, interval_, [this] { return stop_requested_; }));
}

SystemMetrics MetricsService::sample() {
  SystemMetrics metrics;
  metrics.cpu_usage = get_cpu_usage();
  metrics.cpu_temperature = get_cpu_temperature();
  metrics.fan_speed = get_fan_speed();
  metrics.node_name = node_name_;
  return metrics;
}

std::string MetricsService::to_json(const SystemMetrics& metrics)
    const {  // NOLINT(readability-convert-member-functions-to-static)
  Json::Value root;
  root["cpu_usage"] = metrics.cpu_usage;
  root["node_name"] = metrics.node_name;
  root["cpu_temperature"] = metrics.cpu_temperature.has_value()
                                ? Json::Value(*metrics.cpu_temperature)
                                : Json::Value(Json::nullValue);
  root["fan_speed"] = metrics.fan_speed.has_value()
                          ? Json::Value(*metrics.fan_speed)
                          : Json::Value(Json::nullValue);

  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  writer["precision"] = 6;
  return Json::writeString(writer, root);
}

// Platform-specific implementations are in separate files
// - metrics_service_mac.cpp for macOS
// - metrics_service_linux.cpp for Linux

}  // namespace homepage::services
