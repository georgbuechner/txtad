#include "shared/utils/parser/test_file_parser.h"
#include "shared/objects/tests/test_case.h"
#include "shared/utils/utils.h"

std::vector<TestCase> test_parser::LoadTestCases(const std::string& game_id) {
  util::Logger()->info("parser:LoadTestCases: path {}", txtad::GAMES_PATH + game_id + "/" + txtad::GAME_TESTS);
  std::vector<TestCase> test_cases;
  if (auto j_test_cases = util::LoadJsonFromDisc(txtad::GAMES_PATH + game_id + "/" + txtad::GAME_TESTS)) {
    for (const auto& test_case : j_test_cases->get<std::vector<nlohmann::json>>()) {
      test_cases.push_back(TestCase(test_case));
    }
  }
  return test_cases;
}


