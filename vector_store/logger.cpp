#include <iostream>
#include "logger.h"
#include <spdlog/sinks/rotating_file_sink.h>

bool SetupLogger() {
    try {
        auto logger = spdlog::rotating_logger_mt(
                "lgraph_logger", "log/lgraph.log",
                64*1024*1024, 10);
        logger->set_level(spdlog::level::from_str("info"));
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e %t %l %s:%#] %v");
        spdlog::set_default_logger(logger);
        spdlog::flush_every(std::chrono::seconds(3));
    } catch (const std::exception& e) {
        std::cerr << "Log init failed: " << e.what() << std::endl;
        return false;
    }
    return true;
}