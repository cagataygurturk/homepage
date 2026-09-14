#include "controllers/home_controller.hpp"

#include <drogon/HttpViewData.h>

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

namespace homepage::controllers {

namespace {

std::string format_fixed(double value, int decimals) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(decimals) << value;
  return out.str();
}

std::string format_optional(const std::optional<double>& value, int decimals) {
  return value.has_value() ? format_fixed(*value, decimals) : "--";
}

}  // namespace

HomeController::HomeController(services::MetricsService& metricsService)
    : metricsService_(metricsService) {}

void HomeController::index(
    [[maybe_unused]] const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback) const {
  const auto metrics = metricsService_.latest();

  HttpViewData data;
  data.insert("node_name", HttpViewData::htmlTranslate(metrics.node_name));
  data.insert("cpu_usage", format_fixed(metrics.cpu_usage, 1));
  data.insert("cpu_temperature", format_optional(metrics.cpu_temperature, 1));
  data.insert("fan_speed", format_optional(metrics.fan_speed, 0));

  callback(HttpResponse::newHttpViewResponse("home", data));
}

}  // namespace homepage::controllers
