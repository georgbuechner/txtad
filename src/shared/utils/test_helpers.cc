#include "test_helpers.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/utils.h"
#include <vector>

namespace fs = std::filesystem;

test::GameWrapper::GameWrapper(const std::string& name, const nlohmann::json& settings, 
      const std::map<std::string, std::vector<nlohmann::json>>& contexts,
      const std::map<std::string, nlohmann::json>& texts, const nlohmann::json& tests) 
    : _path(txtad::GAMES_PATH + "/" + name) {
  const std::string game_files_path = _path + "/" + txtad::GAME_FILES;
  fs::create_directory(_path);

  // Write settings to disc
  util::WriteJsonToDisc(_path + "/" + txtad::GAME_SETTINGS, settings);
  
  // Write settings to disc
  util::WriteJsonToDisc(_path + "/" + txtad::GAME_TESTS, tests);

  fs::create_directory(game_files_path);
  // Write texts to disc 
  for (const auto& [path, json] : texts) {
    const std::string file_path = game_files_path + ((path !="") ? "/" + path : "");
    if (!fs::exists(file_path)) {
      fs::create_directories(file_path);
    }
    util::WriteJsonToDisc(file_path + txtad::TEXT_EXTENSION, json);
  }

  // Write contexts to disc
  for (const auto& [path, jsons] : contexts) {
    const std::string file_path = game_files_path + ((path !="") ? "/" + path : "");
    if (!fs::exists(file_path)) {
      fs::create_directories(file_path);
    }
    for (const auto& json : jsons)
      util::WriteJsonToDisc(file_path + "/" + json.at("id").get<std::string>() + txtad::CONTEXT_EXTENSION,
          json);
  }
}
test::GameWrapper::~GameWrapper() {
  fs::remove_all(_path);
}

void test::SetAttribute(std::map<std::string, std::string>& attributes, std::string inp, 
    const ExpressionParser& parser) {
  static std::vector<std::string> opts = {"+=", "-=", "++", "--", "*=", "/=", "="}; 
  std::string opt;
  int pos = -1;
  for (const auto& it : opts) {
    auto p = inp.find(it);
    if (p != std::string::npos) {
      opt = it;
      pos = p;
      break;
    }
  }

  if (pos == std::string::npos) {
    util::Logger()->warn("Game::h_set_attribute: no operator found: {}", inp);
    return;
  }

  std::string attribute = inp.substr(0, pos);
  std::string expression = inp.substr(pos+opt.length()); 

  util::Logger()->info("attribute: {}, opt: {}, expression: {}", attribute, opt, expression);

  if (attributes.count(attribute) == 0) {
    return;
  }

  if (opt == "=")
    attributes[attribute] = parser.Evaluate(expression);
  else if (opt == "++")
    attributes[attribute] = std::to_string(std::stoi(attributes.at(attribute)) + 1);
  else if (opt == "--")
    attributes[attribute] = std::to_string(std::stoi(attributes.at(attribute)) - 1);
  else if (opt == "+=")
    attributes[attribute] = std::to_string(std::stoi(attributes.at(attribute)) + std::stoi(parser.Evaluate(expression)));
  else if (opt == "-=")
    attributes[attribute] = std::to_string(std::stoi(attributes.at(attribute)) - std::stoi(parser.Evaluate(expression)));
  else if (opt == "*=")
    attributes[attribute] = std::to_string(std::stoi(attributes.at(attribute)) * std::stoi(parser.Evaluate(expression)));
  else if (opt == "/=")
    attributes[attribute] = std::to_string(std::stoi(attributes.at(attribute)) / std::stoi(parser.Evaluate(expression)));
}

