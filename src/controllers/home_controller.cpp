#include "controllers/home_controller.hpp"

#include <drogon/WebSocketController.h>

#include "templates/index_template.hpp"

namespace homepage::controllers {

HomeController::HomeController() {  // NOLINT(modernize-use-equals-default)
  // Create the response once in constructor
  cached_response_ = HttpResponse::newHttpResponse();
  cached_response_->setContentTypeCode(CT_TEXT_HTML);
  cached_response_->setBody(std::string(homepage::templates::INDEX_HTML));
}

void HomeController::index(
    [[maybe_unused]] const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback) const {
  // Simply return the pre-built response
  callback(cached_response_);
}

}  // namespace homepage::controllers