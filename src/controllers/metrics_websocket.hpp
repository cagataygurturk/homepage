#pragma once

#include <drogon/WebSocketController.h>

namespace homepage::services {
class MetricsService;
}

namespace homepage::controllers {

class MetricsWebSocket
    : public drogon::WebSocketController<MetricsWebSocket, false> {
 public:
  explicit MetricsWebSocket(services::MetricsService& metricsService);
  void handleNewMessage(const drogon::WebSocketConnectionPtr& wsConnPtr,
                        std::string&& message,
                        const drogon::WebSocketMessageType& type) override;

  void handleConnectionClosed(
      const drogon::WebSocketConnectionPtr& wsConnPtr) override;

  void handleNewConnection(
      const drogon::HttpRequestPtr& req,
      const drogon::WebSocketConnectionPtr& wsConnPtr) override;

  WS_PATH_LIST_BEGIN
  WS_PATH_ADD("/ws");
  WS_PATH_LIST_END

 private:
  services::MetricsService& metricsService_;
};

}  // namespace homepage::controllers