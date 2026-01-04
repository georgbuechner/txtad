#ifndef SRC_GAME_UTILS_DEFINES_H
#define SRC_GAME_UTILS_DEFINES_H 

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
  const std::string GAME_TESTS = "tests.json";
  const std::string GAME_FILES = "game_files/";

  // Extentsion
  const std::string BUILDER_EXTENSION = ".builder";
  const std::string TEMPLATE_EXTENSION = ".template";
  const std::string CONTEXT_EXTENSION = ".ctx";
  const std::string TEXT_EXTENSION = ".text";

  // replacements
  const std::string EVENT_REPLACEMENT = "#event";
  const std::string THIS_REPLACEMENT = "<_>";
  const std::string CTX_REPLACEMENT = "<ctx>";
  const std::string UID_REPLACEMENT = "<uid>";
  const std::string IS_USER_REPLACEMENT = "<user-inp>";
  const std::string IS_USER_INP = "^(?!#)(.*)";
  const std::string NO_REPLACEMENT = "#no_subsitute";

  // print ctx regex 
  const std::string RE_PRINT_CTX = R"((.*?)(->|\.)(.*))";

  // GUI cmds 
  const std::string WEB_CMD_CLEAR_CONSOLE = "$clear";
  const std::string WEB_CMD_CALL_DONE = "$done";
  const std::string WEB_CMD_ADD_PROMPT = "$prompt";
  const std::string WEB_COLOR = "$color_";
  const std::string BG_SOUND_PLAY = "$bgsplay"; // played in the background 
  const std::string BG_SOUND_PAUSE = "$bgspause"; // played in the background 
  const std::string BG_SOUND_SET = "$bgsset_"; // played in the background 
  const std::string FG_SOUND = "$fgsound_"; // played and pauses action while playing

  // Other cmds
  const std::string NEW_CONNECTION = "#new_connection";
  const std::string REMOVE_USER = "#remove_user";
};

#endif
