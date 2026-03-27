#ifndef SRC_BUILDER_UTILS_DEFINES_H
#define SRC_BUILDER_UTILS_DEFINES_H 

#include <nlohmann/json.hpp>
#include <set>
#include <string>

namespace builder {
  // Paths
  const std::string LOGGER = "LBuilder";
  const std::string TEST_LOGGER = "LBuilderTest";
  const std::string FILES_PATH = "builder/";
  const std::string CONFIG = FILES_PATH + "builder.config";
  const std::string HTML_PATH = FILES_PATH + "web/";
  const std::string STATIC_PATH = HTML_PATH + "static/";
  const std::string MEDIA_PATH = HTML_PATH + "media/";
  const std::string TEMPLATE_PATH = HTML_PATH + "templates/";
  const std::string CREATORS_PATH = "creators/";

  const std::string COOKIE_NAME = "SESSID";
  const int COOKIE_NAME_LEN = 6;

  const std::string NEW_LISTENER = "[new_listener]";
  const std::string NEW_ATTRIBUTE = "[new_attribute]";

  struct Settings {
    std::string _description;
    std::set<std::string> _hidded_dirs;

    Settings() : _description("") {}
    Settings(nlohmann::json j) : _description(j.value("description", "")), 
      _hidded_dirs(j.value("hidden_dirs", std::set<std::string>())) {}

    nlohmann::json ToJson() const { 
      return {{"description", _description}, {"hidden_dirs", _hidded_dirs}}; 
    }
  };

  enum FileType {
    DIR=0,
    CTX,
    TXT,
    MEDIA,
    TEM
  };
  const std::map<FileType, std::string> FILE_TYPE_MAP = {
    {DIR, "DIR"}, {CTX, "CTX"}, {TXT, "TXT"}, {MEDIA, "MEDIA"}, {TEM, "TEM"}
  };
  const std::map<std::string, FileType> REVERSE_FILE_TYPE_MAP = {
    {"DIR", DIR}, {"CTX", CTX}, {"TXT", TXT}, {"MEDIA", MEDIA}, {"TEM", TEM}
  };

};

#endif
