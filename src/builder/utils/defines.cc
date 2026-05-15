#include "builder/utils/defines.h"
#include "shared/utils/utils.h"

const std::string& builder::LoggerPath() {
  static const std::string path = util::LoadJsonFromDisc(builder::CONFIG)->at("logger_path");
  return path;
}

const std::string& builder::CreatorPath() {
  static const std::string path = util::LoadJsonFromDisc(builder::CONFIG)->at("creators_path");
  return path;
}


