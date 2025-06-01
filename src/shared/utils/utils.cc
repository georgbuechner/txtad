#include "utils.h"
#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::string util::LOGGER = "---";

void util::SetUpLogger(const std::string& main_path, const std::string& name, spdlog::level::level_enum log_level) {
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_st>());
  sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(main_path + "/logs/" + name + "_logfile.txt", 21, 15));
  auto logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
  spdlog::register_logger(logger);
  spdlog::flush_every(std::chrono::seconds(5));
  spdlog::flush_on(spdlog::level::err);
  // set log-level
  logger->set_level(log_level);
}

std::shared_ptr<spdlog::logger> util::Logger() {
  return spdlog::get(LOGGER);
}

std::vector<std::string> util::Split(std::string str, std::string delimiter) {
  std::vector<std::string> v_strs;
  size_t pos=0;
  while ((pos = str.find(delimiter)) != std::string::npos) {
    if(pos!=0)
        v_strs.push_back(str.substr(0, pos));
    str.erase(0, pos + delimiter.length());
  }
  v_strs.push_back(str);
  return v_strs;
}

std::string util::Strip(std::string str, char c) {
  for(;;) {
    if (str.front() == c) 
      str.erase(0, 1);
    else if (str.back() == c) 
      str.pop_back();
    else 
      break;
  }
  return str;
}
