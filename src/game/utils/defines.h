#ifndef SRC_UTILS_DEFINES_H
#define SRC_UTILS_DEFINES_H 

#include <functional>
#include <string>

namespace txtad {
  using MsgFn = std::function<void(std::string)>;

  // Paths
  const std::string LOGGER = "LMain";
  const std::string TEST_LOGGER = "LMainTest";
  const std::string FILES_PATH = "game/";
  const std::string HTML_PATH = "web/";
  const std::string GAMES_PATH = "data/games/";
  const std::string GAME_SETTINGS = "settings.json";
  const std::string GAME_FILES = "game_files/";

  // Extentsion
  const std::string TEMPLATE_EXTENSION = ".template";
  const std::string CONTEXT_EXTENSION = ".ctx";
  const std::string TEXT_EXTENSION = ".txt";

  // replacements
  const std::string THIS_REPLACEMENT = "<_>";
  const std::string CTX_REPLACEMENT = "<ctx>";

  // print ctx regex 
  const std::string RE_PRINT_CTX = R"((.*?)(->|\.)(.*))";
  enum CtxPrint {
    VARIABLE = 0,
    ATTRIBUTE,
  };
};

#endif
