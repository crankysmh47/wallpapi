#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

namespace wp {

class Logger {
public:
    static void init();
    static std::shared_ptr<spdlog::logger>& get_logger();

private:
    static std::shared_ptr<spdlog::logger> s_logger;
};

} // namespace wp

#define WP_TRACE(...) ::wp::Logger::get_logger()->trace(__VA_ARGS__)
#define WP_INFO(...)  ::wp::Logger::get_logger()->info(__VA_ARGS__)
#define WP_WARN(...)  ::wp::Logger::get_logger()->warn(__VA_ARGS__)
#define WP_ERROR(...) ::wp::Logger::get_logger()->error(__VA_ARGS__)
#define WP_CRITICAL(...) ::wp::Logger::get_logger()->critical(__VA_ARGS__)
