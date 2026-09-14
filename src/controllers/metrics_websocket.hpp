#pragma once

#include <drogon/WebSocketController.h>

#include <mutex>
#include <string>
#include <unordered_set>

#include "services/metrics_service.hpp"

namespace homepage::controllers {

// Keeps the set of open WebSocket connections and pushes one JSON message to
// all of them every time the MetricsService produces a new sample. No thread
// is created per connection.
class MetricsWebSocket
    : public drogon::WebSocketController<MetricsWebSocket, false> {
 public:
  explicit MetricsWebSocket(services::MetricsService& metricsService);
  ~MetricsWebSocket() override;

  MetricsWebSocket(const MetricsWebSocket&) = delete;
  MetricsWebSocket& operator=(const MetricsWebSocket&) = delete;
  MetricsWebSocket(MetricsWebSocket&&) = delete;
  MetricsWebSocket& operator=(MetricsWebSocket&&) = delete;

  void handleNewMessage(const drogon::WebSocketConnectionPtr& wsConnPtr,
                        std::string&& message,
                        const drogon::WebSocketMessageType& type) override;

  void handleConnectionClosed(
      const drogon::WebSocketConnectionPtr& wsConnPtr) override;

  void handleNewConnection(
      const drogon::HttpRequestPtr& req,
      const drogon::WebSocketConnectionPtr& wsConnPtr) override;

  // Sends a close frame to every open connection, e.g. before shutdown.
  void closeAll(drogon::CloseCode code, const std::string& reason);

  WS_PATH_LIST_BEGIN
  WS_PATH_ADD("/ws");
  WS_PATH_LIST_END

 private:
  void broadcast(const services::SystemMetrics& metrics);

  services::MetricsService& metricsService_;

  std::mutex connections_mutex_;
  std::unordered_set<drogon::WebSocketConnectionPtr> connections_;

  // Declared last so it is released first, before the members it uses.
  services::MetricsService::SubscriptionId subscription_;
};

}  // namespace homepage::controllers
