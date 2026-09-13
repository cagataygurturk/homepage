#include "metrics_service.hpp"

#ifdef __linux__
#include <sensors/sensors.h>

#include <array>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>

#include "trantor/utils/Logger.h"

namespace homepage::services {

namespace {

// Owns the libsensors global state for the lifetime of the process.
class SensorsLibrary final {
 public:
  SensorsLibrary() : ready_(sensors_init(nullptr) == 0) {
    if (!ready_) {
      LOG_WARN << "libsensors initialisation failed; temperature and fan "
                  "speed will be unavailable";
    }
  }

  SensorsLibrary(const SensorsLibrary&) = delete;
  SensorsLibrary& operator=(const SensorsLibrary&) = delete;
  SensorsLibrary(SensorsLibrary&&) = delete;
  SensorsLibrary& operator=(SensorsLibrary&&) = delete;

  ~SensorsLibrary() {
    if (ready_) {
      sensors_cleanup();
    }
  }

  [[nodiscard]] std::vector<const sensors_chip_name*> chips() const {
    std::vector<const sensors_chip_name*> result;
    if (!ready_) {
      return result;
    }
    int chip_nr = 0;
    while (const sensors_chip_name* chip =
               sensors_get_detected_chips(nullptr, &chip_nr)) {
      result.push_back(chip);
    }
    return result;
  }

  // Returns the first readable input value of the given feature type on the
  // chip, e.g. the first temperature or fan reading.
  [[nodiscard]] std::optional<double> first_input(
      const sensors_chip_name* chip, const sensors_feature_type feature_type,
      const sensors_subfeature_type input_type) const {
    int feature_nr = 0;
    while (const sensors_feature* feature =
               sensors_get_features(chip, &feature_nr)) {
      if (feature->type != feature_type) {
        continue;
      }
      const sensors_subfeature* input =
          sensors_get_subfeature(chip, feature, input_type);
      if (input == nullptr) {
        continue;
      }
      double value = 0.0;
      if (sensors_get_value(chip, input->number, &value) == 0) {
        return value;
      }
    }
    return std::nullopt;
  }

 private:
  bool ready_;
};

const SensorsLibrary& sensors_library() {
  static const SensorsLibrary library;
  return library;
}

// hwmon chip names that report the CPU or SoC temperature, most specific
// first. "cpu_thermal" is the Raspberry Pi thermal zone mirrored into hwmon,
// the others are the AMD and Intel CPU drivers.
constexpr std::array<std::string_view, 4> kCpuChipPrefixes = {
    "cpu_thermal", "k10temp", "coretemp", "zenpower"};

bool is_cpu_chip(const std::string_view prefix) {
  for (const auto known : kCpuChipPrefixes) {
    if (prefix == known) {
      return true;
    }
  }
  return prefix.find("cpu") != std::string_view::npos ||
         prefix.find("soc") != std::string_view::npos;
}

}  // namespace

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
  unsigned long long user = 0;
  unsigned long long nice = 0;
  unsigned long long system = 0;
  unsigned long long idle = 0;
  unsigned long long iowait = 0;
  unsigned long long irq = 0;
  unsigned long long softirq = 0;
  unsigned long long steal = 0;

  iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >>
      softirq >> steal;

  const unsigned long long total =
      user + nice + system + idle + iowait + irq + softirq + steal;
  const unsigned long long total_idle = idle + iowait;

  if (prev_total_ == 0) {
    // First call, store values and return 0
    prev_total_ = total;
    prev_idle_ = total_idle;
    return 0.0;
  }

  const unsigned long long total_diff = total - prev_total_;
  const unsigned long long idle_diff = total_idle - prev_idle_;

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
  const auto& sensors = sensors_library();
  const auto chips = sensors.chips();

  // Walk the preference list first so k10temp wins over an NVMe drive that
  // happens to be enumerated earlier.
  for (const auto preferred : kCpuChipPrefixes) {
    for (const auto* chip : chips) {
      if (chip->prefix != nullptr && preferred == chip->prefix) {
        if (const auto value = sensors.first_input(
                chip, SENSORS_FEATURE_TEMP, SENSORS_SUBFEATURE_TEMP_INPUT)) {
          return value;
        }
      }
    }
  }

  for (const auto* chip : chips) {
    if (chip->prefix != nullptr && is_cpu_chip(chip->prefix)) {
      if (const auto value = sensors.first_input(
              chip, SENSORS_FEATURE_TEMP, SENSORS_SUBFEATURE_TEMP_INPUT)) {
        return value;
      }
    }
  }

  return std::nullopt;
}

std::optional<double> MetricsService::get_fan_speed() {
  for (const auto& sensors = sensors_library();
       const auto* chip : sensors.chips()) {
    if (auto rpm = sensors.first_input(chip, SENSORS_FEATURE_FAN,
                                       SENSORS_SUBFEATURE_FAN_INPUT);
        rpm && *rpm > 0.0) {
      return rpm;
    }
  }

  return std::nullopt;
}

}  // namespace homepage::services

#endif  // __linux__
