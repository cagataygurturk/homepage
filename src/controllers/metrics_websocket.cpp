#include "controllers/metrics_websocket.hpp"

#include <chrono>
#include <thread>

#include "services/metrics_service.hpp"

using namespace drogon;

namespace homepage::controllers {

MetricsWebSocket::MetricsWebSocket(services::MetricsService& metricsService)
    : metricsService_(metricsService) {}

void MetricsWebSocket::handleNewMessage(
    const WebSocketConnectionPtr&
        wsConnPtr,  // NOLINT(readability-convert-member-functions-to-static)
    std::string&& message, const WebSocketMessageType& type) {
  LOG_DEBUG << "Received WebSocket message: " << message;
  // For now, we don't handle client messages
}

void MetricsWebSocket::
    handleConnectionClosed(  // NOLINT(readability-convert-member-functions-to-static)
        const WebSocketConnectionPtr& wsConnPtr) {
  LOG_INFO << "WebSocket connection closed";
}

void MetricsWebSocket::
    handleNewConnection(  // NOLINT(readability-convert-member-functions-to-static,bugprone-easily-swappable-parameters)
        const HttpRequestPtr& req, const WebSocketConnectionPtr& wsConnPtr) {
  LOG_INFO << "New WebSocket connection established";

  // Start sending metrics updates
  std::thread([wsConnPtr, &metricsService = metricsService_]() {
    while (!wsConnPtr->disconnected()) {
      try {
        // Collect metrics
        auto metrics = metricsService.collect();
        std::string json = metricsService.to_json(metrics);

        // Send to client
        if (!wsConnPtr->disconnected()) {
          wsConnPtr->send(json);
        }

        // Wait 2 seconds before next update
        std::this_thread::sleep_for(std::chrono::seconds(2));

      } catch (const std::exception& e) {
        LOG_ERROR << "Error in metrics loop: " << e.what();
        break;
      }
    }

    LOG_INFO << "Metrics loop ended for WebSocket connection";
  }).detach();
}

}  // namespace homepage::controllers