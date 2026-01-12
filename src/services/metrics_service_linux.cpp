#include "metrics_service.hpp"

#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <sstream>

namespace homepage::services {

double MetricsService::get_cpu_usage() {
  // Linux implementation using /proc/stat
  std::ifstream file("/proc/stat");
  if (!file.is_open()) {
    return 0.0;
  }

  std::string line;
  if (!std::getline(file, line)) {
    return 0.0;
  }

  std::istringstream iss(line);
  std::string cpu_label;
  unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;

  iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >>
      softirq >> steal;

  unsigned long long total =
      user + nice + system + idle + iowait + irq + softirq + steal;
  unsigned long long total_idle = idle + iowait;

  if (prev_total_ == 0) {
    // First call, store values and return 0
    prev_total_ = total;
    prev_idle_ = total_idle;
    return 0.0;
  }

  unsigned long long total_diff = total - prev_total_;
  unsigned long long idle_diff = total_idle - prev_idle_;

  double usage = 0.0;
  if (total_diff > 0) {
    usage = 100.0 * static_cast<double>(total_diff - idle_diff) /
            static_cast<double>(total_diff);
  }

  prev_total_ = total;
  prev_idle_ = total_idle;

  return usage;
}

std::optional<double> MetricsService::get_cpu_temperature() {
  try {
    // Check if thermal directory exists first
    if (!std::filesystem::exists("/sys/class/thermal/")) {
      return std::nullopt;
    }

    // Try different thermal zones
    for (const auto& entry :
         std::filesystem::directory_iterator("/sys/class/thermal/")) {
      if (entry.path().filename().string().find("thermal_zone") == 0) {
        std::ifstream temp_file(entry.path() / "temp");
        std::ifstream type_file(entry.path() / "type");

        if (temp_file.is_open() && type_file.is_open()) {
          std::string type;
          type_file >> type;

          // Look for CPU-related thermal zones
          if (type.find("cpu") != std::string::npos ||
              type.find("CPU") != std::string::npos ||
              type.find("x86_pkg_temp") != std::string::npos ||
              type.find("coretemp") != std::string::npos) {
            int temp_millidegrees;
            if (temp_file >> temp_millidegrees) {
              return temp_millidegrees / 1000.0;
            }
          }
        }
      }
    }

    // Try Raspberry Pi thermal zone
    std::ifstream rpi_temp("/sys/class/thermal/thermal_zone0/temp");
    if (rpi_temp.is_open()) {
      int temp_millidegrees;
      if (rpi_temp >> temp_millidegrees) {
        return temp_millidegrees / 1000.0;
      }
    }
  } catch (const std::exception&) {
    // Directory doesn't exist or access error
    return std::nullopt;
  }

  return std::nullopt;
}

std::optional<double> MetricsService::get_fan_speed() {
  // Try hwmon interfaces
  try {
    // Check if hwmon directory exists first
    if (!std::filesystem::exists("/sys/class/hwmon/")) {
      return std::nullopt;
    }

    for (const auto& entry :
         std::filesystem::directory_iterator("/sys/class/hwmon/")) {
      std::string hwmon_path = entry.path();

      // Look for fan speed files
      for (int i = 1; i <= 10; ++i) {
        std::string fan_input =
            hwmon_path + "/fan" + std::to_string(i) + "_input";
        std::ifstream fan_file(fan_input);

        if (fan_file.is_open()) {
          int rpm;
          if (fan_file >> rpm && rpm > 0) {
            return static_cast<double>(rpm);
          }
        }
      }
    }
  } catch (const std::exception&) {
    // Directory might not exist or be accessible
  }

  // Try Raspberry Pi specific fan control
  std::ifstream rpi_fan(
      "/sys/devices/platform/cooling_fan/hwmon/hwmon*/fan1_input");
  if (rpi_fan.is_open()) {
    int rpm;
    if (rpi_fan >> rpm) {
      return static_cast<double>(rpm);
    }
  }

  return std::nullopt;
}

}  // namespace homepage::services

#endif  // __linux__