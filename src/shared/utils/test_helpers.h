#ifndef GAME_TEST_HELPERS_H
#define GAME_TEST_HELPERS_H

#include "game/game/game.h"
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

  void test_random_parser(const ExpressionParser& parser, const std::string& query, const std::string& res, float p);
  void test_random_cout(Game& game, const std::string& user_id, std::string& cout, const std::string& query, 
      std::vector<std::string> options);
}

#endif
