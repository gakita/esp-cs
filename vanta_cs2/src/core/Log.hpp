#pragma once
#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace vanta::log {

// Initializes a rotating file sink under %LOCALAPPDATA%/VantaCS2/logs/vanta.log
// plus a stderr sink. Safe to call once at process start.
void init(const std::string& log_dir);

// Returns the shared logger. Guaranteed non-null after init().
std::shared_ptr<spdlog::logger> get();

} // namespace vanta::log

#define VLOG_TRACE(...) ::vanta::log::get()->trace(__VA_ARGS__)
#define VLOG_INFO(...)  ::vanta::log::get()->info(__VA_ARGS__)
#define VLOG_WARN(...)  ::vanta::log::get()->warn(__VA_ARGS__)
#define VLOG_ERROR(...) ::vanta::log::get()->error(__VA_ARGS__)
