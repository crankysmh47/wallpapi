#include "logger.hpp"
#include <vector>

namespace wp {

std::shared_ptr<spdlog::logger> Logger::s_logger;

void Logger::init() {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("latest.log", true);
    file_sink->set_level(spdlog::level::trace);

    s_logger = std::make_shared<spdlog::logger>("WALLPAPI", file_sink);
#ifdef NDEBUG
    s_logger->set_level(spdlog::level::warn);
#else
    s_logger->set_level(spdlog::level::info);
#endif
    s_logger->flush_on(spdlog::level::warn);

    spdlog::register_logger(s_logger);
}

std::shared_ptr<spdlog::logger>& Logger::get_logger() {
    return s_logger;
}

} // namespace wp
