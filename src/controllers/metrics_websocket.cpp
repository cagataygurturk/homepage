#include "controllers/metrics_websocket.hpp"

#include <vector>

using namespace drogon;

namespace homepage::controllers {

MetricsWebSocket::MetricsWebSocket(services::MetricsService& metricsService)
    : metricsService_(metricsService),
      subscription_(metricsService_.subscribe(
          [this](const services::SystemMetrics& metrics) {
            broadcast(metrics);
          })) {}

MetricsWebSocket::~MetricsWebSocket() {
  metricsService_.unsubscribe(subscription_);
}

void MetricsWebSocket::handleNewMessage(
    const WebSocketConnectionPtr&
        wsConnPtr,  // NOLINT(readability-convert-member-functions-to-static)
    std::string&& message, const WebSocketMessageType& type) {
  LOG_DEBUG << "Received WebSocket message: " << message;
  // For now, we don't handle client messages
}

void MetricsWebSocket::handleConnectionClosed(
    const WebSocketConnectionPtr& wsConnPtr) {
  {
    const std::lock_guard lock(connections_mutex_);
    connections_.erase(wsConnPtr);
  }
  LOG_INFO << "WebSocket connection closed";
}

void MetricsWebSocket::
    handleNewConnection(  // NOLINT(bugprone-easily-swappable-parameters)
        const HttpRequestPtr& req, const WebSocketConnectionPtr& wsConnPtr) {
  LOG_INFO << "New WebSocket connection established";

  {
    const std::lock_guard lock(connections_mutex_);
    connections_.insert(wsConnPtr);
  }

  // Send the current snapshot right away instead of waiting for the next
  // sample.
  wsConnPtr->send(metricsService_.to_json(metricsService_.latest()));
}

void MetricsWebSocket::broadcast(const services::SystemMetrics& metrics) {
  // Copy the set so sends happen without holding the lock. send() is safe to
  // call from any thread; Drogon hands the frame to the connection's loop.
  std::vector<WebSocketConnectionPtr> connections;
  {
    const std::lock_guard lock(connections_mutex_);
    connections.assign(connections_.begin(), connections_.end());
  }
  if (connections.empty()) {
    return;
  }

  const std::string json = metricsService_.to_json(metrics);
  for (const auto& connection : connections) {
    if (!connection->disconnected()) {
      connection->send(json);
    }
  }
}

}  // namespace homepage::controllers
