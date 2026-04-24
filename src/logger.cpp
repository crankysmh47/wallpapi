#include "logger.hpp"
#include <vector>

namespace wp {

std::shared_ptr<spdlog::logger> Logger::s_logger;

void Logger::init() {
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("latest.log", true));

    s_logger = std::make_shared<spdlog::logger>("WALLPAPI", sinks.begin(), sinks.end());
    s_logger->set_level(spdlog::level::trace);
    s_logger->flush_on(spdlog::level::trace);

    spdlog::register_logger(s_logger);
}

std::shared_ptr<spdlog::logger>& Logger::get_logger() {
    return s_logger;
}

} // namespace wp
