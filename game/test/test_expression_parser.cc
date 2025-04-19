#include "utils/fuzzy_search/fuzzy.h"
#include "utils/parser/expression_parser.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("Text expression parser", "[parser]") {
  ExpressionParser parser;
  // From function description
  REQUIRE(parser.evaluate("Hund ~ Hündin") == "0");
  REQUIRE(parser.evaluate("Hund ~ hund") == "1");
  REQUIRE(parser.evaluate("Hund ~ Hunde") == "2");
  REQUIRE(parser.evaluate("Hund ~ JahrHUNDert") == "3");
  REQUIRE(parser.evaluate("Mimesis ~ Mimisis") == "4");
  REQUIRE(parser.evaluate("book:[bottle; lighter; book]") == "1");
  REQUIRE(parser.evaluate("tabako:[bottle; lighter; book]") == "0");
  REQUIRE(parser.evaluate("[tabako|lighter]:[bottle; lighter; book]") == "1");
  REQUIRE(parser.evaluate("tobako~:[Bottle; Lighter; Tabako; Book]") == "[4]");
  REQUIRE(parser.evaluate("tobako~:[Bottle; Lighter; Tabako; Book] = [4]") == "1");
  
  // whitespace handling
  REQUIRE(parser.evaluate(" 10 + 10") == "20");
  REQUIRE(parser.evaluate("10 + 10") == "20");
  REQUIRE(parser.evaluate("10+ 10") == "20");
  REQUIRE(parser.evaluate("10 +10") == "20");
  REQUIRE(parser.evaluate("10+10") == "20");
  REQUIRE(parser.evaluate("Ha lo=Halo") == "0");
  REQUIRE(parser.evaluate("Ha lo=Ha lo") == "1");
  REQUIRE(parser.evaluate("Ha lo = Ha lo") == "1");
  REQUIRE(parser.evaluate("10 : [ 5; ;720;10;8; 72 ;5]") == "1");
  REQUIRE(parser.evaluate("2010 : [ 5; ;720;10;8; 72 ;5]") == "0");
  REQUIRE(parser.evaluate("[ 10 | 11 ] : [5;720;11;8;72]") == "1");

  // math
  REQUIRE(parser.evaluate("10 + 10") == "20");
  REQUIRE(parser.evaluate("10 - 10") == "0");
  REQUIRE(parser.evaluate("10 * 10") == "100");
  REQUIRE(parser.evaluate("10 / 10") == "1");
  REQUIRE(parser.evaluate("10 + 10 + 10") == "30");
  REQUIRE(parser.evaluate("10 - 10 + 10") == "10");
  REQUIRE(parser.evaluate("10 * 10 - 10 + 5") == "95");
  REQUIRE(parser.evaluate("10 / 10 - 10+5 * 5 ") == "-20");
  REQUIRE(parser.evaluate("10 / 5 * 2 ") == "4");  

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
  REQUIRE(parser.evaluate("10 : [10]") == "1");
  REQUIRE(parser.evaluate("10 : [10;5]") == "1");
  REQUIRE(parser.evaluate("10 : [5;10]") == "1");
  REQUIRE(parser.evaluate("10 : [5;720;10;8;72]") == "1");
  REQUIRE(parser.evaluate("10 : [5;720;11;8;72]") == "0");
  REQUIRE(parser.evaluate("[10|11] : [5;720;11;8;72]") == "1");
  REQUIRE(parser.evaluate("10 < 100") == "1");
  REQUIRE(parser.evaluate("10 > 100") == "0");
  REQUIRE(parser.evaluate("10 > 9") == "1");
  REQUIRE(parser.evaluate("10 < 9") == "0");
  REQUIRE(parser.evaluate("10 < 9") == "0");
  REQUIRE(parser.evaluate("10 >= 9") == "1");
  REQUIRE(parser.evaluate("10 >= 10") == "1");
  REQUIRE(parser.evaluate("10 >= 11") == "0");
  REQUIRE(parser.evaluate("10 <= 11") == "1");
  REQUIRE(parser.evaluate("10 <= 10") == "1");
  REQUIRE(parser.evaluate("10 <= 9") == "0");
  std::string res = parser.evaluate("mimisis ~: [Eingedenken; das Hinzutretende; Mimesis; Leid]");
  REQUIRE(res == "[" + std::to_string(fuzzy::FuzzyMatch::FUZZY) + "]");
  REQUIRE(parser.evaluate(std::to_string(fuzzy::FuzzyMatch::FUZZY) + ":"+res) == "1"); // 4:[...]
  res = parser.evaluate("Mimesis ~: [Eingedenken; mimesisch; das Hinzutretende; Leid]");
  REQUIRE(res == "[" + std::to_string(fuzzy::FuzzyMatch::STARTS_WITH) + "]");
  REQUIRE(parser.evaluate("[" + std::to_string(fuzzy::FuzzyMatch::FUZZY) + "|" 
        + std::to_string(fuzzy::FuzzyMatch::STARTS_WITH) + "]:"+res) == "1"); // [4|2] : [...]

  // Does not work, requires brackets!
  // REQUIRE(parser.evaluate(std::to_string(fuzzy::FuzzyMatch::FUZZY) + ":" 
  //       + "mimisis ~: [Eingedenken; das Hinzutretende; Mimesis; Leid]") == "1");
  REQUIRE(parser.evaluate("mimisis ~: [Eingedenken; das Hinzutretende; Leid]") == "[]");

  REQUIRE(parser.evaluate("Hund ~ hund = " + std::to_string(fuzzy::FuzzyMatch::DIRECT)) == "1");
  REQUIRE(parser.evaluate("Mimesis ~ mimisis =" + std::to_string(fuzzy::FuzzyMatch::FUZZY)) == "1");
  REQUIRE(parser.evaluate("hündin ~ jahrhundert =" + std::to_string(fuzzy::FuzzyMatch::NO_MATCH)) == "1");
  REQUIRE(parser.evaluate("hündin ~ jahrhundert =" + std::to_string(fuzzy::FuzzyMatch::FUZZY)) == "0");

  // brackets 
  REQUIRE(parser.evaluate("4 - ( 2 + 2)") == "0");
}
