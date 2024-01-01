#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
#include "spdlog/cfg/env.h"
std::shared_ptr<spdlog::logger> initialize_logger() {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logfile.txt");
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // Tạo logger với cả hai sinks
    auto logger = std::make_shared<spdlog::logger>("logger", spdlog::sinks_init_list{console_sink, file_sink});

    // Đặt mức độ log
    logger->set_level(spdlog::level::debug);

    return logger;
}
