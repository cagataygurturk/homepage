#pragma once

#include <optional>
#include <string>

namespace homepage::services {

struct SystemMetrics {
  double cpu_usage = 0.0;
  std::optional<double> cpu_temperature;
  std::optional<double> fan_speed;
  std::string node_name;
};

class MetricsService final {
 public:
  MetricsService();

  SystemMetrics collect();

  [[nodiscard]] std::string to_json(const SystemMetrics& metrics) const;

 private:
  double get_cpu_usage();
  std::optional<double> get_cpu_temperature();
  std::optional<double> get_fan_speed();

  // For CPU usage calculation
  unsigned long long prev_total_ = 0;
  unsigned long long prev_idle_ = 0;
};

}  // namespace homepage::services