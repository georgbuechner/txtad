/**
 * @author: fux
 */
#ifndef SHARED_UTILS_UTIL_H
#define SHARED_UTILS_UTIL_H

#include "game/utils/defines.h"
#include <exception>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <regex>
#include <set>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <string>

namespace util
{
  namespace fs = std::filesystem;

  class invalid_base_class_call : public std::exception {
    private: 
      std::string _msg; 
    public: 
      invalid_base_class_call(const char* msg) : _msg(msg) { }

      const char* what() const noexcept {
        return _msg.c_str();
      }
  };

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

  std::string Join(const std::vector<std::string>& vec, const std::string& separator);

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

  void RemoveEmptyDirs(const std::string& path);

  /** 
   * Trys to find user-id in given string. If found retruns user id and removes
   * from original string.
   * @param[in, out] inp 
   * @returns user-id or nullopt
   */
  std::optional<std::string> GetUserId(std::string& inp);

  /** 
   * Simple validation, that can only handled json's which are objects and top
   * level.
   */
  std::optional<std::map<std::string, nlohmann::json>> ValidateSimpleJson(std::string json_string, std::vector<std::string> keys);

  /**
   * Creates a random string (chars: 0-9, a-z, A-Z) from given length
   * @param[in] len 
   * @return random string (chars: 0-9, a-z, A-Z)
   */
  std::string CreateRandomString(int len);

  /**
   * @brief Implements a cryptographic hash function, uses the slower but better
   * SHA3 algorithm also named Keccak
   * @param input The string to hash, for example password email whatever.
   * @return The hashed string is returned, the input remains unchanged
   */
  std::string HashSha3512(const std::string& input);

  std::string iso_utc_now();

  std::string generate_random_hex_string(size_t length);

  bool IsIdType(const std::string& id);

  std::string GetPage(const std::string& path);

  std::vector<std::string> GetSubpaths(const std::vector<std::string>& ids);
  std::vector<std::string> GetSubpaths(const std::string& id);

  bool IsSubPathOf(const std::string& path, const std::string& sub);
  bool IsSubPathOf(const std::vector<std::string>& paths, const std::string& sub);

  std::string GetContentType(const std::string& extension);


  class TmpPath {
    public: 
      TmpPath(const std::vector<std::string>& dirs = {}) : _path("data/tmp/" + CreateRandomString(8)) {
        fs::create_directory(_path);
        for (const auto& it : dirs) {
          fs::create_directory(_path + "/" + it);
        }
      }
      ~TmpPath() {
        fs::remove_all(_path);
      }
      const std::string& get() { return _path; }

    private: 
      const std::string _path;
  };

  /**
   * Custom regex class applying basic escaping and stores string
   * representation.
   * Escaping: 
   * - '*' -> '\*'
   */
  class Regex {
    public: 
      Regex(const std::string& pattern) : _pattern(pattern), _regex(std::regex(pattern)) {
        static const std::string IS_USER_INP = "^(?!#)(.*)";
        std::string new_pattern = "";
        for (int i=0; i<pattern.length(); i++) {
          if (pattern[i] == '*' && (i == 0 || (pattern[i-1] != '.' && pattern[i-1] != '\\')))
            new_pattern += "\\*";
          else 
            new_pattern += pattern[i];
        }
        new_pattern = ReplaceAll(new_pattern, txtad::IS_USER_REPLACEMENT, IS_USER_INP);
        _regex = std::regex(new_pattern);
        _pattern = pattern;
      }

      const std::string& str() const { return _pattern; } 
      operator const std::regex&() const { return _regex; } 
    private: 
      std::string _pattern;
      std::regex _regex;
  };

  template< typename tPair >
  struct second_t {
      typename tPair::second_type operator()( const tPair& p ) const { return p.second; }
  };

  template< typename tMap > 
  second_t< typename tMap::value_type > second( const tMap& m ) { 
    return second_t< typename tMap::value_type >(); }

  template<typename T1, typename T2> 
  std::shared_ptr<T2> get_ptr(const std::map<T1, std::shared_ptr<T2>>& m, const T1& key) {
    auto it = m.find(key);
    return (it != m.end()) ? it->second : nullptr;
  }
}

#endif
