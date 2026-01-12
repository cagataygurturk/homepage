#include "metrics_service.hpp"

#ifdef __APPLE__
#include <mach/host_info.h>
#include <mach/mach.h>
#include <mach/mach_host.h>

namespace homepage::services {

double MetricsService::get_cpu_usage() {
  // macOS implementation using Mach APIs
  host_cpu_load_info_data_t cpu_info;
  mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

  if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                      reinterpret_cast<host_info_t>(&cpu_info),
                      &count) != KERN_SUCCESS) {
    return 0.0;
  }

  unsigned long long total = cpu_info.cpu_ticks[CPU_STATE_USER] +
                             cpu_info.cpu_ticks[CPU_STATE_SYSTEM] +
                             cpu_info.cpu_ticks[CPU_STATE_IDLE] +
                             cpu_info.cpu_ticks[CPU_STATE_NICE];
  unsigned long long idle = cpu_info.cpu_ticks[CPU_STATE_IDLE];

  if (prev_total_ == 0) {
    // First call, store values and return 0
    prev_total_ = total;
    prev_idle_ = idle;
    return 0.0;
  }

  unsigned long long total_diff = total - prev_total_;
  unsigned long long idle_diff = idle - prev_idle_;

  double usage = 0.0;
  if (total_diff > 0) {
    usage = 100.0 * static_cast<double>(total_diff - idle_diff) /
            static_cast<double>(total_diff);
  }

  prev_total_ = total;
  prev_idle_ = idle;

  return usage;
}

std::optional<double> MetricsService::get_cpu_temperature() {
  // macOS doesn't expose CPU temperature via standard APIs
  // Would require IOKit framework for hardware sensors
  return std::nullopt;
}

std::optional<double> MetricsService::get_fan_speed() {
  // macOS doesn't expose fan speed via standard APIs
  // Would require IOKit framework for hardware sensors
  return std::nullopt;
}

}  // namespace homepage::services

#endif  // __APPLE__