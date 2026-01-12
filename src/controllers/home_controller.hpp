#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace homepage::controllers {

class HomeController : public HttpController<HomeController> {
 public:
  HomeController();

  METHOD_LIST_BEGIN
  // Route for homepage
  ADD_METHOD_TO(HomeController::index, "/", Get);
  METHOD_LIST_END

  void index(const HttpRequestPtr& req,
             std::function<void(const HttpResponsePtr&)>&& callback) const;

 private:
  HttpResponsePtr cached_response_;
};

}  // namespace homepage::controllers