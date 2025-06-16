#ifndef SRC_UTILS_DEFINES_H
#define SRC_UTILS_DEFINES_H 

#include <functional>
#include <string>

namespace txtad {
  using MsgFn = std::function<void(std::string)>;
  using SubstituteFN = std::function<std::string(std::string)>;

  const std::string LOGGER = "LMain";
  const std::string TEST_LOGGER = "LMainTest";
  const std::string FILES_PATH = "game/";
  const std::string HTML_PATH = "web/";
  const std::string GAMES_PATH = "data/games/";
  const std::string GAME_SETTINGS = "settings.json";
  const std::string GAME_FILES = "game_files/";

  const std::string TEMPLATE_EXTENSION = ".template";
  const std::string CONTEXT_EXTENSION = ".ctx";
  const std::string TEXT_EXTENSION = ".txt";

};

#endif
