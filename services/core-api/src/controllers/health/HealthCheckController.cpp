#include <drogon/HttpController.h>
#include "HealthCheckController.h"

using namespace drogon;
using namespace std;
namespace api {
    void HealthCheckController::health(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(HttpStatusCode::k200OK);
        resp->setContentTypeCode(ContentType::CT_APPLICATION_JSON);
        resp->setBody("{\"status\": \"ok\", \"service\": \"core-api\"}");
        callback(resp);
    };
};