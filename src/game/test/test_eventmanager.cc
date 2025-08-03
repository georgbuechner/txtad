#include "game/test/helpers.h"
#include "game/utils/defines.h"
#include "shared/utils/eventmanager/eventmanager.h"
#include "shared/utils/eventmanager/listener.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/utils.h"
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <memory>
#include <string>

void TakeEvents(std::string& events, EventManager& em, ExpressionParser& parser) {
  util::Logger()->debug("TakeEvents: \"{}\"", events);
  if (events == "") 
    return;
  auto vec_events = util::Split(events, ";");
  for (const auto& it : vec_events) {
    util::Logger()->debug("TakeEvents: - \"{}\"", it);
  }
  events = "";
  for (const auto& event : vec_events) {
    em.TakeEvent(event, parser);
  }
}

TEST_CASE("Test eventmanager basic use", "[eventmanager]") {
  const std::string E_SET_MANA = "#sa mana=20";
  const std::string E_SET_RUNES = "#sa runes=100";

  EventManager em; 
  std::map<std::string, std::string> attributes = {{"mana", "10"}, {"runes", "5"}, {"called", "0"}};
  ExpressionParser::SubstituteFN fn = [&attributes](const std::string& str) { 
    return (attributes.count(str) > 0) ? attributes.at(str) : ""; };
  ExpressionParser parser(fn);
  std::string event_queue = "";

  Listener::Fn set_attribute = [&attributes, &parser](std::string event, std::string args) {
    helpers::SetAttribute(attributes, args, parser);
  };

  Listener::Fn add_to_eventqueue = [&event_queue](std::string event, std::string args) {
    event_queue += ((event_queue != "") ? ";" : "") + args;
  };

  LForwarder::set_overwite_fn(add_to_eventqueue);

  SECTION ("Test re-adding events") {
    // Basic handlers
    em.AddListener(std::make_shared<LHandler>("M1", "#sa (.*)", set_attribute));

    // Custom handlers
    em.AddListener(std::make_shared<LForwarder>("P1", "go", E_SET_MANA, true));
    em.AddListener(std::make_shared<LForwarder>("P2", "talk", E_SET_MANA, true));
    em.AddListener(std::make_shared<LForwarder>("P3", "talk", E_SET_RUNES, true));

    em.TakeEvent("talk", parser);
    REQUIRE(event_queue == E_SET_MANA + ";" + E_SET_RUNES);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["mana"] == "20");
    REQUIRE(attributes["runes"] == "100");
  }

  SECTION ("Test event forwarding") {
    em.AddListener(std::make_shared<LHandler>("M1", "#sa (.*)", set_attribute));
    em.AddListener(std::make_shared<LForwarder>("P1", "<user-inp>", "#sa called=1", true, 
          "#event ~ elephant = {fuzzy}"));

    // "cat" fails
    em.TakeEvent("cat", parser);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["called"] == "0");
    // "elefant" succeeds (fuzzy match)
    em.TakeEvent("elefant", parser);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["called"] == "1");

    // forwarding works multiple times:
    em.AddListener(std::make_shared<LForwarder>("P1", "<user-inp>", "#sa called=0", true, 
          "(elephant ~ #event = {fuzzy}) && (#event ~ elephant = {fuzzy})"));
    em.TakeEvent("elefant", parser);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["called"] == "0");
    
    // forwarding works for arguments too:
    em.AddListener(std::make_shared<LForwarder>("P1", "<user-inp>", "#sa called=#event", true, 
          "elephant ~ #event = {fuzzy}"));
    em.TakeEvent("elefant", parser);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["called"] == "elefant");
  }

  SECTION ("Test event avoiding problems with spacing") {
    em.AddListener(std::make_shared<LHandler>("M1", "#sa (.*)", set_attribute));
    em.AddListener(std::make_shared<LForwarder>("P1", txtad::IS_USER_REPLACEMENT, 
          "#sa called++; #sa called++", true, "#event ~ elephant = {fuzzy}"));

    // "cat" fails
    em.TakeEvent("cat", parser);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["called"] == "0");
    // "elefant" succeeds (fuzzy match)
    em.TakeEvent("elefant", parser);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["called"] == "2");
  }

  SECTION("Test escaping regex chars") {
    em.AddListener(std::make_shared<LHandler>("M1", "#sa (.*)", set_attribute));
    em.AddListener(std::make_shared<LForwarder>("P1", "#sa drinks+=(.*)", "#sa called++", true));
    em.AddListener(std::make_shared<LForwarder>("P1", "#sa smokes\\+=(.*)", "#sa called++", true));

    em.TakeEvent("#sa drinks+=10", parser);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["called"] == "0");
    em.TakeEvent("#sa smokes+=10", parser);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["called"] == "1");

  }
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
  ExpressionParser::SubstituteFN fn = [&attributes](const std::string& str) { 
    util::Logger()->info("SUBSTITUTE-FN: str: {}, mana: {} runes: {}", str, attributes.at("mana"), attributes.at("runes"));
    return (attributes.count(str) > 0) ? attributes.at(str) : ""; };
  ExpressionParser parser(fn);
  std::string event_queue = "";

  Listener::Fn set_attribute = [&attributes, &parser](std::string event, std::string args) {
    helpers::SetAttribute(attributes, args, parser);
  };

  Listener::Fn add_to_eventqueue = [&event_queue](std::string event, std::string args) {
    event_queue += ((event_queue != "") ? ";" : "") + args;
  };
  LForwarder::set_overwite_fn(add_to_eventqueue);

  // Basic handlers
  em.AddListener(std::make_shared<LHandler>("M1", "#sa (.*)", set_attribute));

  // Custom handlers
  em.AddListener(std::make_shared<LForwarder>("P1", "set", E_SET_MANA, true));
  em.AddListener(std::make_shared<LForwarder>("P2", "add", E_ADD_MANA, true));
  em.AddListener(std::make_shared<LForwarder>("P3", "inc", E_INC_MANA, true));
  em.AddListener(std::make_shared<LForwarder>("P4", "dec", E_DEC_MANA, true));
  em.AddListener(std::make_shared<LForwarder>("P5", "set_to", E_SET_RUNES_TO_MANA, true));
  em.AddListener(std::make_shared<LForwarder>("P6", "inc_by", E_INC_RUNES_BY_DOUBLE_MANA, true));

  em.TakeEvent("set", parser);
  em.TakeEvent(event_queue, parser);
  event_queue = "";
  REQUIRE(attributes["mana"] == "20");
  em.TakeEvent("add", parser);
  em.TakeEvent(event_queue, parser);
  event_queue = "";
  REQUIRE(attributes["mana"] == "40");
  em.TakeEvent("inc", parser);
  em.TakeEvent(event_queue, parser);
  event_queue = "";
  REQUIRE(attributes["mana"] == "41");
  em.TakeEvent("dec", parser);
  em.TakeEvent(event_queue, parser);
  event_queue = "";
  REQUIRE(attributes["mana"] == "40");
  em.TakeEvent("set_to", parser);
  em.TakeEvent(event_queue, parser);
  event_queue = "";
  REQUIRE(attributes["runes"] == attributes["mana"]);
  em.TakeEvent("inc_by", parser);
  em.TakeEvent(event_queue, parser);
  event_queue = "";
  REQUIRE(attributes["runes"] == "120");
}

TEST_CASE("Test permeability", "[eventmanager]") {
  const std::string E_SET_MANA = "#sa mana=20";
  const std::string E_SET_RUNES = "#sa runes=100";

  EventManager em; 
  std::map<std::string, std::string> attributes = {{"mana", "10"}, {"runes", "5"}};
  ExpressionParser::SubstituteFN fn = [&attributes](const std::string& str) { 
    return (attributes.count(str) > 0) ? attributes.at(str) : ""; };
  ExpressionParser parser(fn);
  std::string event_queue = "";

  Listener::Fn set_attribute = [&attributes, &parser](std::string event, std::string args) {
    helpers::SetAttribute(attributes, args, parser);
  };
  Listener::Fn add_to_eventqueue = [&event_queue](std::string event, std::string args) {
    event_queue += ((event_queue != "") ? ";" : "") + args;
  };
  LForwarder::set_overwite_fn(add_to_eventqueue);

  // Basic handlers
  em.AddListener(std::make_shared<LHandler>("M1", "#sa (.*)", set_attribute));

  // Custom handlers
  em.AddListener(std::make_shared<LForwarder>("P2", "talk", E_SET_MANA, false));
  em.AddListener(std::make_shared<LForwarder>("P3", "talk", E_SET_RUNES, true));

  em.TakeEvent("talk", parser);
  REQUIRE(event_queue == E_SET_MANA);
  TakeEvents(event_queue, em, parser);
  REQUIRE(attributes["mana"] == "20");
  REQUIRE(attributes["runes"] == "5");
}

TEST_CASE("Test logic", "[eventmanager]") {
  const std::string E_SET_MANA = "#sa mana=20";
  const std::string E_SET_RUNES = "#sa runes=100";

  EventManager em; 
  std::map<std::string, std::string> attributes = {{"mana", "10"}, {"runes", "5"}};
  ExpressionParser::SubstituteFN fn = [&attributes](const std::string& str) { 
    return (attributes.count(str) > 0) ? attributes.at(str) : ""; };
  ExpressionParser parser(fn);
  std::string event_queue = "";

  Listener::Fn set_attribute = [&attributes, &parser](std::string event, std::string args) {
    helpers::SetAttribute(attributes, args, parser);
  };
  Listener::Fn add_to_eventqueue = [&event_queue](std::string event, std::string args) {
    event_queue += ((event_queue != "") ? ";" : "") + args;
  };
  LForwarder::set_overwite_fn(add_to_eventqueue);

  // Basic handlers
  em.AddListener(std::make_shared<LHandler>("M1", "#sa (.*)", set_attribute));

  SECTION("First invalid") {
    // Custom handlers
    em.AddListener(std::make_shared<LForwarder>("P2", "talk", E_SET_MANA, true, "{mana} = 10"));
    em.AddListener(std::make_shared<LForwarder>("P3", "talk", E_SET_RUNES, true, "{mana} = 5"));

    em.TakeEvent("talk", parser);
    REQUIRE(event_queue == E_SET_MANA);
  }

  SECTION("Second invalid") {
    // Custom handlers
    em.AddListener(std::make_shared<LForwarder>("P2", "talk", E_SET_MANA, true, "{mana} > 10"));
    em.AddListener(std::make_shared<LForwarder>("P3", "talk", E_SET_RUNES, true, "{mana} >= 10"));

    em.TakeEvent("talk", parser);
    REQUIRE(event_queue == E_SET_RUNES);
  }

  SECTION("Both invalid") {
    // Custom handlers
    em.AddListener(std::make_shared<LForwarder>("P2", "talk", E_SET_MANA, true, "{mana} ~ zehn == {direct}"));
    em.AddListener(std::make_shared<LForwarder>("P3", "talk", E_SET_RUNES, true, "{mana} != 10"));

    em.TakeEvent("talk", parser);
    REQUIRE(event_queue == "");
  }

  SECTION("Both valid") {
    // Custom handlers
    em.AddListener(std::make_shared<LForwarder>("P2", "talk", E_SET_MANA, true, "{mana} <= 10"));
    em.AddListener(std::make_shared<LForwarder>("P3", "talk", E_SET_RUNES, true, "{mana} >= 10"));

    em.TakeEvent("talk", parser);
    REQUIRE(event_queue == fmt::format("{};{}", E_SET_MANA, E_SET_RUNES));
  }

  SECTION("Update while throwing (priority high enougth)") {
    // Custom handlers
    em.AddListener(std::make_shared<LForwarder>("P2", "talk", E_SET_MANA, true, "{mana} = 10"));
    em.AddListener(std::make_shared<LForwarder>("P3", "talk", E_SET_RUNES, true, "{mana} = 5"));
    em.AddListener(std::make_shared<LForwarder>("G1", "#sa .*", E_SET_RUNES, true, "{mana} = 10"));

    em.TakeEvent("talk", parser);
    REQUIRE(event_queue == E_SET_MANA);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["mana"] == "20");
    REQUIRE(event_queue == E_SET_RUNES); 
  }


  SECTION("Update while throwing (priority not high enouth for start-values)") {
    // Custom handlers
    em.AddListener(std::make_shared<LForwarder>("P2", "talk", E_SET_MANA, true, "{mana} = 10"));
    em.AddListener(std::make_shared<LForwarder>("P3", "talk", E_SET_RUNES, true, "{mana} = 5"));
    em.AddListener(std::make_shared<LForwarder>("P4", "#sa .*", E_SET_RUNES, true, "{mana} = 10"));

    em.TakeEvent("talk", parser);
    REQUIRE(event_queue == E_SET_MANA);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["mana"] == "20");
    REQUIRE(event_queue == ""); // -> did not work because r4 has lower priority
                                // than 1 (the default set-handler
  }

  SECTION("Update while throwing (priority not high enouth, but using correct values)") {
    // Custom handlers
    em.AddListener(std::make_shared<LForwarder>("P2", "talk", E_SET_MANA, true, "{mana} = 10"));
    em.AddListener(std::make_shared<LForwarder>("P3", "talk", E_SET_RUNES, true, "{mana} = 5"));
    em.AddListener(std::make_shared<LForwarder>("P4", "#sa .*", E_SET_RUNES, true, "{mana} = 20"));

    em.TakeEvent("talk", parser);
    REQUIRE(event_queue == E_SET_MANA);
    TakeEvents(event_queue, em, parser);
    REQUIRE(attributes["mana"] == "20");
    REQUIRE(event_queue == E_SET_RUNES); 
  }
}
