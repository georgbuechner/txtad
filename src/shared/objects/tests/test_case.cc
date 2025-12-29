#include "game/game/game.h"
#include "shared/objects/tests/test_case.h"


TestCase::TestCase(const nlohmann::json json) : _desc(json.at("desc")) {
  for (const auto& test : json.at("tests")) {
    std::cout << "Loading : " << test.dump() << std::endl;
    _tests.push_back(Test(test));
  }
}

// methods 

std::string TestCase::Run(std::shared_ptr<Game> game) const {
  static const std::string user_id = "XX";
  std::string res = "";
  Game::set_msg_fn([&res](const std::string&, const std::string& msg) { 
      std::cout << "GOT MSG: " << msg << std::endl;
      res = msg; 
  });

  for (const auto& test : _tests) {
    Test::_t_test_result test_res = test.Run(game, user_id, res);
    if (!test_res.first) {
      return test_res.second;
    }
  }
  return "";
}
