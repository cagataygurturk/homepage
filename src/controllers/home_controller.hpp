#pragma once

#include <drogon/HttpController.h>

#include "services/metrics_service.hpp"

using namespace drogon;

namespace homepage::controllers {

// Renders the CSP view in views/home.csp with the latest metrics snapshot,
// so the page shows real values before the WebSocket connects.
class HomeController : public HttpController<HomeController, false> {
 public:
  explicit HomeController(services::MetricsService& metricsService);

  METHOD_LIST_BEGIN
  // Route for homepage
  ADD_METHOD_TO(HomeController::index, "/", Get);
  METHOD_LIST_END

  void index(const HttpRequestPtr& req,
             std::function<void(const HttpResponsePtr&)>&& callback) const;

 private:
  services::MetricsService& metricsService_;
};

}  // namespace homepage::controllers
