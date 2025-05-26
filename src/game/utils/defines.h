#ifndef SRC_UTILS_DEFINES_H
#define SRC_UTILS_DEFINES_H 

#include <functional>
#include <string>

namespace txtad {
  using MsgFn = std::function<void(std::string)>;

  const std::string LOGGER = "LMain";
  const std::string TEST_LOGGER = "LMainTest";
  const std::string FILES_PATH = "game/";
  const std::string HTML_PATH = "web/";
  const std::string GAMES_PATH = "data/games/";
};

#endif
