#include <drogon/drogon.h>
using namespace drogon;

int main() {

  uint16_t port = std::getenv("PORT") ? std::stoi(std::getenv("PORT")) : 8080;

  app()
      .setLogPath("./logs")
      .setLogLevel(trantor::Logger::kInfo)
      .addListener("0.0.0.0", port)
      .setThreadNum(2)
      .run();

  return 0;
}