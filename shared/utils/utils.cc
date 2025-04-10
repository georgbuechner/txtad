#include "utils/utils.h"
#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void util::SetUpLogger(const std::string& name, spdlog::level::level_enum log_level) {
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_st>());
  sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/" + name + "_logfile.txt", 21, 15));
  auto logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
  spdlog::register_logger(logger);
  spdlog::flush_every(std::chrono::seconds(5));
  spdlog::flush_on(spdlog::level::err);
  // set log-level
  logger->set_level(log_level);
}
