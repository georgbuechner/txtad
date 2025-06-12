/**
 * @author: fux
 */
#ifndef SHARED_UTILS_UTIL_H
#define SHARED_UTILS_UTIL_H

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <regex>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <string>

namespace util
{
  void SetUpLogger(const std::string& main_path, const std::string& name, spdlog::level::level_enum log_level);
  std::shared_ptr<spdlog::logger> Logger();

  extern std::string LOGGER;

  class LoggerContext {
    public: 
      LoggerContext(std::string logger) {
        _prev = LOGGER;
        LOGGER = logger;
      }
      ~LoggerContext() {
        LOGGER = _prev;
      }
    private: 
      std::string _prev;
  };

  class Regex {
    public: 
      Regex(const std::string& pattern) : _pattern(pattern), _regex(std::regex(pattern)) {}
      const std::string& str() const { return _pattern; } 
      operator const std::regex&() const { return _regex; } 
    private: 
      std::string _pattern;
      std::regex _regex;
  };

  std::vector<std::string> Split(std::string str, std::string delimiter);

  /**
   * Removes all leading and trailing occurances of designated character.
   * @param[in] str to stip 
   * @param[in] char 
   * @return striped string
   */
  std::string Strip(std::string str, char c=' ');

  std::optional<nlohmann::json> LoadJsonFromDisc(const std::string& path);

  void WriteJsonToDisc(const std::string& path, const nlohmann::json& json);
}

#endif
