/**
 * @author: fux
 */
#ifndef SHARED_UTILS_UTIL_H
#define SHARED_UTILS_UTIL_H

#include <spdlog/common.h>
#include <string>

namespace util
{
  void SetUpLogger(const std::string& name, spdlog::level::level_enum log_level);

  std::vector<std::string> Split(std::string str, std::string delimiter);

  /**
   * Removes all leading and trailing occurances of designated character.
   * @param[in] str to stip 
   * @param[in] char 
   * @return striped string
   */
  std::string Strip(std::string str, char c=' ');
}

#endif
