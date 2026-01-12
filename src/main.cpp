#include <drogon/drogon.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <trantor/utils/Logger.h>

#include <iostream>

#include "controllers/home_controller.hpp"
#include "controllers/metrics_websocket.hpp"
#include "services/metrics_service.hpp"
#include "utils/config.hpp"

using namespace drogon;

int main() {
  // Configure spdlog with JSON formatting before any logging
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

  // Set JSON pattern for structured logging
  console_sink->set_pattern(
      R"({"timestamp":"%Y-%m-%dT%H:%M:%S.%f","level":"%l","thread":"%t","logger":"%n","message":"%v"})");

  // Create logger
  auto logger = std::make_shared<spdlog::logger>("drogon", console_sink);
  logger->set_level(spdlog::level::info);

  // Enable spdlog for Drogon/Trantor logging
  trantor::Logger::enableSpdLog(logger);

  // Create MetricsService instance
  static auto metricsService =
      std::make_unique<homepage::services::MetricsService>();

  // Force WebSocket controller registration
  static auto ws_controller =
      std::make_shared<homepage::controllers::MetricsWebSocket>(
          *metricsService);

  // Get configuration
  auto& config = homepage::utils::Config::instance();

  LOG_INFO << "Starting homepage server on " << config.getAddress() << ":"
           << config.getPort();

  // Configure server
  app().addListener(config.getAddress(), config.getPort());

  // Controllers auto-register their routes via WS_PATH_LIST
  app().registerController(ws_controller);

  // Set document root for static files if needed
  // app().setDocumentRoot("./public");

  // Configure JSON parsing
  app().setClientMaxBodySize(1024 * 1024);  // 1MB max body size

  LOG_INFO << "Server started successfully";
  LOG_INFO << "Environment variables:";
  LOG_INFO << "  PORT=" << config.getPort();
  LOG_INFO << "  ADDRESS=" << config.getAddress();

  // Start the server
  app().run();

  return 0;
}