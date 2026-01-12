#include "metrics_service.hpp"

#include <sstream>

namespace homepage::services {

MetricsService::MetricsService() {
  // Initialize CPU stats for first reading
  get_cpu_usage();
}

SystemMetrics MetricsService::collect() {
  SystemMetrics metrics;

  metrics.cpu_usage = get_cpu_usage();
  metrics.cpu_temperature = get_cpu_temperature();
  metrics.fan_speed = get_fan_speed();

  // Get node name from environment variable (set by Kubernetes Downward API)
  const char* node_name = std::getenv("NODE_NAME");
  metrics.node_name = (node_name != nullptr) ? node_name : "unknown";

  return metrics;
}

std::string MetricsService::to_json(const SystemMetrics& metrics)
    const {  // NOLINT(readability-convert-member-functions-to-static)
  std::ostringstream json;
  json << "{";
  json << "\"cpu_usage\":" << metrics.cpu_usage;
  json << R"(,"node_name":")" << metrics.node_name << R"(")";

  if (metrics.cpu_temperature.has_value()) {
    json << ",\"cpu_temperature\":" << metrics.cpu_temperature.value();
  }

  if (metrics.fan_speed.has_value()) {
    json << ",\"fan_speed\":" << metrics.fan_speed.value();
  }

  json << "}";
  return json.str();
}

// Platform-specific implementations are in separate files (Chromium
// convention):
// - metrics_service_mac.cpp for macOS
// - metrics_service_linux.cpp for Linux

}  // namespace homepage::services