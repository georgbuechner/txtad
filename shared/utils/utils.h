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
}

#endif
