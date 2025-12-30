#include "game/game/game.h"
#include "game/utils/defines.h"
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

  util::TmpPath tmp_path({txtad::GAME_FILES});
  game->StoreGame(tmp_path.get());

  Game tmp_game(tmp_path.get(), "tmp_game");
  tmp_game.set_msg_fn([&res](const std::string&, const std::string& msg) { res = msg; });
   
  // Create test-user 
  tmp_game.HandleEvent(user_id, txtad::NEW_CONNEXTION);

  for (const auto& test : _tests) {
    Test::_t_test_result test_res = test.Run(tmp_game, user_id, res);
    if (!test_res.first) {
      return test_res.second;
    }
  }
  return "";
}
