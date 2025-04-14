#include "utils/defines.h"
#include "utils/eventmanager/eventmanager.h"test
#include "utils/eventmanager/listener.h"
#include "utils/utils.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <vector>

TEST_CASE("Test eventmanager basic use", "[eventmanager]") {
  const std::string E_SET_MANA = "#sa mana=20";
  const std::string E_SET_RUNES = "#sa runes=100";

  EventManager em; 
  std::map<std::string, int> attributes = {{"mana", 10}, {"runes", 5}};
  std::string event_queue = "";

  Listener::Fn set_attribute = [&attributes](std::string event, std::string args) {
    std::string attribute = args.substr(0, args.find("="));
    int val = std::stoi(args.substr(args.find("=")+1));
    if (attributes.count(attribute) > 0) 
      attributes[attribute] = val; 
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
  REQUIRE(attributes["mana"] == 20);
  REQUIRE(attributes["runes"] == 100);
}

TEST_CASE("Test permeability", "[eventmanager]") {
  const std::string E_SET_MANA = "#sa mana=20";
  const std::string E_SET_RUNES = "#sa runes=100";

  EventManager em; 
  std::map<std::string, int> attributes = {{"mana", 10}, {"runes", 5}};
  std::string event_queue = "";

  Listener::Fn set_attribute = [&attributes](std::string event, std::string args) {
    std::string attribute = args.substr(0, args.find("="));
    int val = std::stoi(args.substr(args.find("=")+1));
    if (attributes.count(attribute) > 0) 
      attributes[attribute] = val; 
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
  REQUIRE(attributes["mana"] == 20);
  REQUIRE(attributes["runes"] == 5);
}
