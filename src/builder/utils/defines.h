#ifndef SRC_BUILDER_UTILS_DEFINES_H
#define SRC_BUILDER_UTILS_DEFINES_H 

#include <string>

namespace builder {
  // Paths
  const std::string LOGGER = "LBuilder";
  const std::string TEST_LOGGER = "LBuilderTest";
  const std::string FILES_PATH = "builder/";
  const std::string HTML_PATH = FILES_PATH + "web/";
  const std::string STATIC_PATH = HTML_PATH + "static/";
  const std::string MEDIA_PATH = HTML_PATH + "media/";
  const std::string TEMPLATE_PATH = HTML_PATH + "templates/";
  const std::string CREATORS_PATH = "creators/";

  const std::string COOKIE_NAME = "SESSID";
  const int COOKIE_NAME_LEN = 6;
};

#endif
