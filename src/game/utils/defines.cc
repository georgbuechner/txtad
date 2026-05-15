#include "defines.h"
#include "builder/utils/defines.h"
#include "shared/utils/utils.h"

const std::string& txtad::GamesPath() {
  static const std::string path = util::LoadJsonFromDisc(builder::CONFIG)->at("txtad").at("games_path");
  return path;
}

const std::string& txtad::LoggerPath() {
  static const std::string path = util::LoadJsonFromDisc(builder::CONFIG)->at("txtad").at("logger_path");
  return path;
}
