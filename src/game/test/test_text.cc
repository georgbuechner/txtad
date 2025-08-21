#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json_fwd.hpp>
#include "shared/objects/text/text.h"
#include "shared/utils/utils.h"

TEST_CASE("Test creating text from string only", "[text]") {
  const std::string TXT = "Hello world";

  std::string event_queue = "";
  Text txt(TXT);

  ExpressionParser::SubstituteFN fn = [](const std::string& str) { return ""; };
  ExpressionParser parser(fn);

  REQUIRE(txt.print(event_queue, parser).front() == TXT);
  REQUIRE(event_queue == "");
}

TEST_CASE("Test creating text with events", "[text]") {
  const std::string TXT = "Hello world";
  const std::string ONE_TIME_EVENTS = "#print texts.first";
  const std::string PERMANENT_EVENTS = "#sa hp=10";

  std::string event_queue = "";
  Text txt(TXT, ONE_TIME_EVENTS, PERMANENT_EVENTS);

  ExpressionParser::SubstituteFN fn = [](const std::string& str) { return ""; };
  ExpressionParser parser(fn);

  REQUIRE(txt.print(event_queue, parser).front() == TXT);
  REQUIRE(event_queue == PERMANENT_EVENTS + ";" + ONE_TIME_EVENTS);
  event_queue = "";
  REQUIRE(txt.print(event_queue, parser).front() == TXT);
  REQUIRE(event_queue == PERMANENT_EVENTS);
}

TEST_CASE("Test creating single text from json", "[text]") {
  const std::string TXT = "Hello world";
  const std::string ONE_TIME_EVENTS = "#print texts.first";
  const std::string PERMANENT_EVENTS = "#sa hp=10";
  const bool SHARED = false;

  nlohmann::json json = {{"shared", SHARED}, {"txt", TXT}, {"one_time_events", ONE_TIME_EVENTS}, 
    {"permanent_events", PERMANENT_EVENTS}};

  std::string event_queue = "";
  Text txt(json);

  ExpressionParser::SubstituteFN fn = [](const std::string& str) { return ""; };
  ExpressionParser parser(fn);

  REQUIRE(txt.shared() == SHARED);

  REQUIRE(txt.print(event_queue, parser).front() == TXT);
  REQUIRE(event_queue == PERMANENT_EVENTS + ";" + ONE_TIME_EVENTS);
  event_queue = "";
  REQUIRE(txt.print(event_queue, parser).front() == TXT);
  REQUIRE(event_queue == PERMANENT_EVENTS);
}

TEST_CASE("Test creating multiple texts from json", "[text]") {
  const std::string TXT = "Hello world";
  const bool SHARED = false;

  nlohmann::json json = nlohmann::json::array();
  json.push_back({{"shared", SHARED}, {"txt", util::Split(TXT, " ")[0]}});  // "hello"
  json.push_back({{"txt", util::Split(TXT, " ")[1]}});                      // "world"

  std::string event_queue = "";
  Text txt(json);

  ExpressionParser::SubstituteFN fn = [](const std::string& str) { return ""; };
  ExpressionParser parser(fn);

  REQUIRE(txt.shared() == SHARED);

  REQUIRE(util::Join(txt.print(event_queue, parser), " ") == TXT);
}

TEST_CASE("Test multiple texts, but some don't match logic", "[text]") {
  const std::string TXT = "Hello dumm world";
  const bool SHARED = false;

  nlohmann::json json = nlohmann::json::array();
  json.push_back({{"shared", SHARED}, {"txt", util::Split(TXT, " ")[0]}});  // "hello" []
  json.push_back({{"txt", util::Split(TXT, " ")[1]}, {"logic", "1=2"}});    // "dumm" [logic: `1==4`]
  json.push_back({{"txt", util::Split(TXT, " ")[2]}, {"logic", "1=1"}});    // "world" [logic: `1==1`]

  std::string event_queue = "";
  Text txt(json);

  ExpressionParser::SubstituteFN fn = [](const std::string& str) { return ""; };
  ExpressionParser parser(fn);

  REQUIRE(txt.shared() == SHARED);

  REQUIRE(util::Join(txt.print(event_queue, parser), " ") == util::Split(TXT, " ")[0] + " " + util::Split(TXT, " ")[2]);
}
