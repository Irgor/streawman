#include "../src/controllers/health/HealthCheckController.h"
#include <drogon/RequestStream.h>
#include <gtest/gtest.h>

TEST(HealthCheckControllerTests, ReturnsOkStatusAndJsonBody) {
  api::HealthCheckController controller;
  HttpRequestPtr req = drogon::HttpRequest::newHttpRequest();

  drogon::HttpResponsePtr captured;
  controller.health(req, [&captured](const drogon::HttpResponsePtr &resp) {
    captured = resp;
  });
  ASSERT_EQ(captured->statusCode(), drogon::k200OK);
  ASSERT_EQ(captured->body(), "{\"status\": \"ok\", \"service\": \"core-api\"}");
}