#include "core/Log.hpp"

#include <filesystem>
#include <mutex>
#include <spdlog/sinks/dist_sink.h>

namespace vanta::log {
namespace {
std::shared_ptr<spdlog::logger> g_logger;
std::once_flag g_once;
}

void init(const std::string& log_dir) {
    std::call_once(g_once, [&] {
        std::filesystem::create_directories(log_dir);
        auto dist = std::make_shared<spdlog::sinks::dist_sink_mt>();

        auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_dir + "/vanta.log", /*max_size=*/5 * 1024 * 1024, /*max_files=*/4);
        file->set_level(spdlog::level::trace);
        dist->add_sink(file);

        auto console = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        console->set_level(spdlog::level::info);
        dist->add_sink(console);

        g_logger = std::make_shared<spdlog::logger>("vanta", dist);
        g_logger->set_level(spdlog::level::trace);
        g_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        g_logger->flush_on(spdlog::level::warn);
        spdlog::register_logger(g_logger);
    });
}

std::shared_ptr<spdlog::logger> get() {
    if (!g_logger) {
        init(".");
    }
    return g_logger;
}

} // namespace vanta::log
