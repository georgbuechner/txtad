#include "builder/game/builder_game.h"
#include "game/utils/defines.h"
#include "shared/utils/utils.h"
#include <exception>
#include <vector>
#include "shared/objects/tests/test_case.h"
#include "builder/game/builder_game.h"


TestCase::TestCase() : _desc(""), _tests(std::vector<Test>()) {
  _tests.push_back(Test());
}
TestCase::TestCase(const nlohmann::json& json) : _desc(json.at("desc").get<std::string>()) {
  _tests = std::vector<Test>();
  for (const auto& test : json.at("tests")) {
    _tests.push_back(Test(test));
  }
}

// getter
const std::string& TestCase::desc() const {
  return _desc;
}
const std::vector<Test>& TestCase::tests() const {
  return _tests;
}

// methods 

std::string TestCase::Run(std::shared_ptr<BuilderGame> game) const {
  static const std::string user_id = "XX";
  std::string res = "";

  util::TmpPath tmp_path({txtad::GAME_FILES});
  game->StoreGame(tmp_path.get());

  try {
    BuilderGame tmp_game(tmp_path.get(), "tmp_game");
    tmp_game.set_msg_fn([&res](const std::string&, const std::string& msg) { res = msg; });

    // Create test-user 
    tmp_game.HandleEvent(user_id, txtad::NEW_CONNECTION);

    for (const auto& test : _tests) {
      Test::_t_test_result test_res = test.Run(tmp_game, user_id, res);
      if (!test_res.first) {
        return test_res.second;
      }
    }
  } catch (std::exception& e) {
    std::string msg = "Failed executing tests: " + std::string(e.what());
    util::Logger()->error("TestCase::Run: " + msg);
    return msg;
  }
  return "";
}
