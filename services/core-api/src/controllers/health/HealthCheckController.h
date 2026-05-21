#pragma once
#include <drogon/HttpController.h>

using namespace drogon;
using namespace std;
namespace api {
    class HealthCheckController : public drogon::HttpController<HealthCheckController> {
        public:
        METHOD_LIST_BEGIN
        ADD_METHOD_TO(HealthCheckController::health, "/health", Get);
        METHOD_LIST_END

        void health(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback);
    };
}