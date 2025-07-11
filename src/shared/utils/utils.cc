#include "utils.h"
#include <exception>
#include <fmt/core.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
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

std::string util::ReplaceAll(std::string str, const std::string& from, const std::string& to) {
  size_t pos = 0;
  while ((pos = str.find(from, pos)) != std::string::npos) {
    str.replace(pos, from.length(), to);
    pos += to.length();
  }
  return str;
}

std::pair<int, int> util::InBrackets(const std::string& inp, int pos) {
  return {OpeningBracket(inp, pos), ClosingBracket(inp, pos)};
}


int util::ClosingBracket(const std::string& inp, int pos, char open, char close) {
  int accept = 0;
  int end = -1;
  for (int i=0; i<inp.length()-pos; i++) {
    char c = inp[pos+i];
    if (c == open) 
      accept++;
    else if (c == close && accept > 0) 
      accept--;
    else if (c == close && accept == 0) {
      end = pos+i;
      break;
    }
  }
  return end;
}

int util::OpeningBracket(const std::string& inp, int pos, char open, char close) {
  int accept = 0;
  int start = -1; 
  for (int i=0; i<=pos; i++) {
    char c = inp[pos-i];
    if (c == close) 
      accept++;
    else if (c == open && accept > 0) 
      accept--;
    else if (c == open && accept == 0) {
      start = pos-i;
      break;
    }
  }
  return start;
}

std::optional<nlohmann::json> util::LoadJsonFromDisc(const std::string& path) {
  try {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
      Logger()->error("Failed loading json from disc: input file could not be opened: {}", path);
    } else {
      auto json = nlohmann::json::parse(ifs);
      ifs.close();
      return json;
    }
  } catch (std::exception& e) {
    Logger()->error("Failed to load json ({}) from disc: {}", path, e.what());
  }
  return std::nullopt;
}

void util::WriteJsonToDisc(const std::string& path, const nlohmann::json& json) {
  std::ofstream ofs(path);
  if (!ofs.is_open()) {
    Logger()->error("Failed writing json to disc: output file could not be opened!");
  } else {
    ofs << json;
    ofs.close();
  }
}

std::optional<std::string> util::GetUserId(std::string& inp) {
  if (inp.find("0x7") == 0) {
    std::string user_id = inp.substr(0, 14);
    inp = inp.substr(14, inp.length()-14);
    return user_id;
  }
  return std::nullopt;
}
