#include <drogon/drogon.h>


int main() {

    drogon::app()
        .setLogPath("./logs")
        .setLogLevel(trantor::Logger::kInfo)
        .addListener("0.0.0.0", 8080)
        .setThreadNum(2)
        .run();

    return 0;
}