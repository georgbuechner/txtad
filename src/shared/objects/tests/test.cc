#include "builder/game/builder_game.h"
#include "game/game/game.h"
#include "test.h"

Test::Test() : _cmd("..."), _result(), _checks() {}
Test::Test(const nlohmann::json& test) : _cmd(test.at("cmd").get<std::string>()), 
  _result(test.value("result", "")), _checks(test.value("checks", std::vector<std::string>())) { }
// getter
std::string Test::cmd() const { 
  return _cmd;
}
std::string Test::result() const { 
  return _result;
}
const std::vector<std::string>& Test::checks() const {
  return _checks;
}

// Methods
Test::_t_test_result Test::Run(BuilderGame& game, const std::string& user_id, std::string& res) const {
  try {
    game.HandleEvent(user_id, _cmd);
  } catch (std::exception& e) {
    return {false, fmt::format("{}: Handling event failed with error \"{}\"", _cmd, e.what())};
  }

  // Check direct return-result:
  if (_result != "") {
    if (_result != res) {
      return {false, fmt::format("{}: {} != {}", _cmd, res, _result)};
    }
  } 
  
  // Check for attributes: 
  for (const auto& logic  : _checks) {
    std::string lres = game.CheckLogic(logic);
    if (lres != "1") {
      return {false, fmt::format("{}: {} evaluated to {}", _cmd, logic, lres)};
    }
  }
  return {true, ""};
}
