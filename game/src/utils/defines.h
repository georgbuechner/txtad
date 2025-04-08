#ifndef SRC_UTILS_DEFINES_H
#define SRC_UTILS_DEFINES_H 

#include <functional>
#include <string>

namespace txtad {
  using MsgFn = std::function<void(std::string)>;

  const std::string LOGGER = "LMain";

};

#endif
