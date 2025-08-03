#include <catch2/catch_test_macros.hpp>
#include "shared/objects/text/text.h"

TEST_CASE("Test printing text", "[text]") {
  const std::string TXT = "Hello world";
  const std::string ONE_TIME_EVENTS = "#print texts.first";
  const std::string PERMANENT_EVENTS = "#sa hp=10";

  std::string event_queue = "";
  Text txt(TXT, ONE_TIME_EVENTS, PERMANENT_EVENTS);

  REQUIRE(txt.print(event_queue) == TXT);
  REQUIRE(event_queue == PERMANENT_EVENTS + ";" + ONE_TIME_EVENTS);
  event_queue = "";
  REQUIRE(txt.print(event_queue) == TXT);
  REQUIRE(event_queue == PERMANENT_EVENTS);
}
