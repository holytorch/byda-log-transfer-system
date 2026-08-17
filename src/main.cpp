#include <iostream>
#include <memory>
#include "util/log/logger.h"

int main() {
    Logger::init();
    
    try {
        LOG_INFO("application started");
    } catch (const std::exception& e) {
        LOG_CRITICAL("unhandled exception: {}", e.what());
        Logger::shutdown();
        return -1;
    }
    
    Logger::shutdown();
    return 0;
}