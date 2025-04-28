#include "utils/eventmanager/eventmanager.h"
#include "utils/eventmanager/listener.h"
#include "utils/parser/expression_parser.h"
#include "utils/utils.h"
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <memory>
#include <regex>
#include <string>

void SetAttribute(std::map<std::string, std::string>& attributes, std::string inp) {
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

  util::Logger()->info(fmt::format("attribute: {}, opt: {}, expression: {}", attribute, opt, expression));

  if (attributes.count(attribute) == 0) {
    return;
  }

  ExpressionParser parser(attributes);

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

TEST_CASE("Test eventmanager basic use", "[eventmanager]") {
  const std::string E_SET_MANA = "#sa mana=20";
  const std::string E_SET_RUNES = "#sa runes=100";

  EventManager em; 
  std::map<std::string, std::string> attributes = {{"mana", "10"}, {"runes", "5"}};
  std::string event_queue = "";

  Listener::Fn set_attribute = [&attributes](std::string event, std::string args) {
    SetAttribute(attributes, args);
  };

  Listener::Fn add_to_eventqueue = [&event_queue](std::string event, std::string args) {
    event_queue += ((event_queue != "") ? ";" : "") + args;
  };

  // Basic handlers
  em.AddListener(std::make_shared<Listener>("1", std::regex("#sa (.*)"), "", set_attribute, true));

  // Custom handlers
  em.AddListener(std::make_shared<Listener>("r1", std::regex("go"), E_SET_MANA, 
        add_to_eventqueue, true));
  em.AddListener(std::make_shared<Listener>("r2", std::regex("talk"), E_SET_MANA, 
        add_to_eventqueue, true));
  em.AddListener(std::make_shared<Listener>("r3", std::regex("talk"), E_SET_RUNES, 
        add_to_eventqueue, true));

  em.TakeEvent("talk");
  REQUIRE(event_queue == E_SET_MANA + ";" + E_SET_RUNES);
  for (const auto& event : util::Split(event_queue, ";"))
    em.TakeEvent(event);
  REQUIRE(attributes["mana"] == "20");
  REQUIRE(attributes["runes"] == "100");
}

TEST_CASE("Test eventmanager: SetAttribute", "[eventmanager]") {
  const std::string E_SET_MANA = "#sa mana=20";
  const std::string E_ADD_MANA = "#sa mana+=20";
  const std::string E_INC_MANA = "#sa mana++";
  const std::string E_DEC_MANA = "#sa mana--";
  const std::string E_SET_RUNES_TO_MANA = "#sa runes={mana}";
  const std::string E_INC_RUNES_BY_DOUBLE_MANA = "#sa runes+={mana}*2";

  EventManager em; 
  std::map<std::string, std::string> attributes = {{"mana", "10"}, {"runes", "5"}};
  std::string event_queue = "";

  Listener::Fn set_attribute = [&attributes](std::string event, std::string args) {
    SetAttribute(attributes, args);
  };

  Listener::Fn add_to_eventqueue = [&event_queue](std::string event, std::string args) {
    event_queue += ((event_queue != "") ? ";" : "") + args;
  };

  // Basic handlers
  em.AddListener(std::make_shared<Listener>("1", std::regex("#sa (.*)"), "", set_attribute, true));

  // Custom handlers
  em.AddListener(std::make_shared<Listener>("r1", std::regex("set"), E_SET_MANA, 
        add_to_eventqueue, true));
  em.AddListener(std::make_shared<Listener>("r2", std::regex("add"), E_ADD_MANA, 
        add_to_eventqueue, true));
  em.AddListener(std::make_shared<Listener>("r3", std::regex("inc"), E_INC_MANA, 
        add_to_eventqueue, true));
  em.AddListener(std::make_shared<Listener>("r4", std::regex("dec"), E_DEC_MANA, 
        add_to_eventqueue, true));
  em.AddListener(std::make_shared<Listener>("r5", std::regex("set_to"), E_SET_RUNES_TO_MANA, 
        add_to_eventqueue, true));
  em.AddListener(std::make_shared<Listener>("r6", std::regex("inc_by"), E_INC_RUNES_BY_DOUBLE_MANA, 
        add_to_eventqueue, true));

  em.TakeEvent("set");
  em.TakeEvent(event_queue);
  event_queue = "";
  REQUIRE(attributes["mana"] == "20");
  em.TakeEvent("add");
  em.TakeEvent(event_queue);
  event_queue = "";
  REQUIRE(attributes["mana"] == "40");
  em.TakeEvent("inc");
  em.TakeEvent(event_queue);
  event_queue = "";
  REQUIRE(attributes["mana"] == "41");
  em.TakeEvent("dec");
  em.TakeEvent(event_queue);
  event_queue = "";
  REQUIRE(attributes["mana"] == "40");
  em.TakeEvent("set_to");
  em.TakeEvent(event_queue);
  event_queue = "";
  REQUIRE(attributes["runes"] == attributes["mana"]);
  em.TakeEvent("inc_by");
  em.TakeEvent(event_queue);
  event_queue = "";
  REQUIRE(attributes["runes"] == "120");


}

TEST_CASE("Test permeability", "[eventmanager]") {
  const std::string E_SET_MANA = "#sa mana=20";
  const std::string E_SET_RUNES = "#sa runes=100";

  EventManager em; 
  std::map<std::string, std::string> attributes = {{"mana", "10"}, {"runes", "5"}};
  std::string event_queue = "";

  Listener::Fn set_attribute = [&attributes](std::string event, std::string args) {
    SetAttribute(attributes, args);
  };
  Listener::Fn add_to_eventqueue = [&event_queue](std::string event, std::string args) {
    event_queue += ((event_queue != "") ? ";" : "") + args;
  };

  // Basic handlers
  em.AddListener(std::make_shared<Listener>("1", std::regex("#sa (.*)"), "", set_attribute, true));

  // Custom handlers
  em.AddListener(std::make_shared<Listener>("r2", std::regex("talk"), E_SET_MANA, 
        add_to_eventqueue, false));
  em.AddListener(std::make_shared<Listener>("r3", std::regex("talk"), E_SET_RUNES, 
        add_to_eventqueue, true));

  em.TakeEvent("talk");
  REQUIRE(event_queue == E_SET_MANA);
  for (const auto& event : util::Split(event_queue, ";"))
    em.TakeEvent(event);
  REQUIRE(attributes["mana"] == "20");
  REQUIRE(attributes["runes"] == "5");
}
