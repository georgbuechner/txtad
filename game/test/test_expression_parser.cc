#include "utils/fuzzy_search/fuzzy.h"
#include "utils/parser/expression_parser.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("Text expression parser", "[parser]") {
  ExpressionParser parser;
  // whitespace handling
  REQUIRE(parser.evaluate(" 10 + 10") == "20");
  REQUIRE(parser.evaluate("10 + 10") == "20");
  REQUIRE(parser.evaluate("10+ 10") == "20");
  REQUIRE(parser.evaluate("10 +10") == "20");
  REQUIRE(parser.evaluate("10+10") == "20");
  REQUIRE(parser.evaluate("Ha lo=Halo") == "0");
  REQUIRE(parser.evaluate("Ha lo=Ha lo") == "1");
  REQUIRE(parser.evaluate("Ha lo = Ha lo") == "1");
  // REQUIRE(parser.evaluate("10 : [ 5, ,720,10,8, 72 ,5]") == "1");
  // REQUIRE(parser.evaluate("2010 : [ 5, ,720,10,8, 72 ,5]") == "0");
  // math
  REQUIRE(parser.evaluate("10 + 10") == "20");
  REQUIRE(parser.evaluate("10 - 10") == "0");
  REQUIRE(parser.evaluate("10 * 10") == "100");
  REQUIRE(parser.evaluate("10 / 10") == "1");
  REQUIRE(parser.evaluate("10 + 10 + 10") == "30");
  REQUIRE(parser.evaluate("10 - 10 + 10") == "10");
  REQUIRE(parser.evaluate("10 * 10 - 10 + 5") == "95");
  REQUIRE(parser.evaluate("10 / 10 - 10+5 * 5 ") == "-20");

  // comparision
  REQUIRE(parser.evaluate("10 = 10") == """1");
  REQUIRE(parser.evaluate("10 = 11") == """0");
  REQUIRE(parser.evaluate("10 ~ 100") == std::to_string(fuzzy::FuzzyMatch::STARTS_WITH));
  REQUIRE(parser.evaluate("10 ~ 110") == std::to_string(fuzzy::FuzzyMatch::CONTAINS));
  REQUIRE(parser.evaluate("Hund ~ hund") == std::to_string(fuzzy::FuzzyMatch::DIRECT));
  REQUIRE(parser.evaluate("hund ~ Hund") == std::to_string(fuzzy::FuzzyMatch::DIRECT));
  REQUIRE(parser.evaluate("hund ~ hunde") == std::to_string(fuzzy::FuzzyMatch::STARTS_WITH));
  REQUIRE(parser.evaluate("hunde ~ hund") == std::to_string(fuzzy::FuzzyMatch::NO_MATCH));
  REQUIRE(parser.evaluate("hund ~ jahrhundert") == std::to_string(fuzzy::FuzzyMatch::CONTAINS));
  REQUIRE(parser.evaluate("hündin ~ jahrhundert") == std::to_string(fuzzy::FuzzyMatch::NO_MATCH));
  REQUIRE(parser.evaluate("Mimesis ~ mimisis") == std::to_string(fuzzy::FuzzyMatch::FUZZY));
  //REQUIRE(parser.evaluate("10 : [5, ,720,10,8, 72 ,]") == "1");
  //REQUIRE(parser.evaluate("10 : [5, ,720,11,8, 72 ,]") == "0");
  REQUIRE(parser.evaluate("10 < 100") == "1");
  REQUIRE(parser.evaluate("10 > 100") == "0");
  REQUIRE(parser.evaluate("10 > 9") == "1");
  REQUIRE(parser.evaluate("10 < 9") == "0");
  REQUIRE(parser.evaluate("10 < 9") == "0");
}
