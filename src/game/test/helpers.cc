#include "helpers.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/utils.h"
#include <vector>

void helpers::SetAttribute(std::map<std::string, std::string>& attributes, std::string inp, const ExpressionParser& parser) {
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

