/**
 * @author: fux
 */
#ifndef SHARED_UTILS_UTIL_H
#define SHARED_UTILS_UTIL_H

#include <iostream>
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
      Regex(const std::string& pattern) : _pattern(pattern), _regex(std::regex(pattern)) {
        std::string new_pattern = "";
        for (int i=0; i<pattern.length(); i++) {
          if (pattern[i] == '*' && (i == 0 || pattern[i-1] != '.'))
            new_pattern += "\\*";
          else 
            new_pattern += pattern[i];
        }
        _regex = std::regex(new_pattern);
        _pattern = new_pattern;
      }

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

  /** 
   * Replace all ocurences of `from` with `to` and return modified string 
   * @param[in] str (source string) 
   * @param[in] from (string to find and replace) 
   * @param[in] to (string to replace `from` with)
   * @return modified string
   */
  std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);


  /**
   * If the char at the given position is surrounded by brackets, return their 
   * start and end position.
   * @param[in] inp 
   * @param[in] pos 
   * @return start and end positions of surrounding brackets
   */
  std::pair<int, int> InBrackets(const std::string& inp, int pos);

  /**
   * Find next (right side) closing bracket
   * @param[in] inp (logical expression) 
   * @param[in] pos (current position to search from)
   * @return position of closing bracket (-1 if it does not exist)
   */
  int ClosingBracket(const std::string& inp, int pos, char open = '(', char close = ')');

  /**
   * Find previous (left side) opening bracket
   * @param[in] inp (logical expression) 
   * @param[in] pos (current position to search from)
   * @return position of opening bracket (-1 if it does not exist)
   */
  int OpeningBracket(const std::string& inp, int pos, char open = '(', char close = ')');


  std::optional<nlohmann::json> LoadJsonFromDisc(const std::string& path);

  void WriteJsonToDisc(const std::string& path, const nlohmann::json& json);

  /** 
   * Trys to find user-id in given string. If found retruns user id and removes
   * from original string.
   * @param[in, out] inp 
   * @returns user-id or nullopt
   */
  std::optional<std::string> GetUserId(std::string& inp);
}

#endif
