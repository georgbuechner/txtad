#ifndef GAME_TEST_HELPERS_H
#define GAME_TEST_HELPERS_H

#include "shared/utils/parser/expression_parser.h"
#include <map>
#include <nlohmann/json.hpp>
#include <string>

namespace test {
  class GameWrapper {
    private:
      const std::string _path;

    public: 
      GameWrapper(const std::string& name, const nlohmann::json& settings, 
            const std::map<std::string, std::vector<nlohmann::json>>& contexts,
            const std::map<std::string, nlohmann::json>& texts, 
            const nlohmann::json& tests = {});
      ~GameWrapper();
  };

  void SetAttribute(std::map<std::string, std::string>& attributes, std::string inp, const ExpressionParser& parser);
}

#endif
