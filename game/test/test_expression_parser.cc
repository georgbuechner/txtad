#include "utils/fuzzy_search/fuzzy.h"
#include "utils/parser/expression_parser.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("Text expression parser", "[parser]") {
  // Assure substrings
  std::string str = "2 (1+1) + 2";
  auto start = str.find("(");
  auto end = str.find(")");
  REQUIRE(str.substr(0, start) == "2 ");
  REQUIRE(str.substr(start+1, end-start-1) == "1+1");
  REQUIRE(str.substr(end+1, str.length()-end) == " + 2");

  ExpressionParser parser;
  REQUIRE(parser.Evaluate("10 * 10") == "100");
  REQUIRE(parser.Evaluate("10*2") == "20");
  REQUIRE(parser.Evaluate("20+10*2") == "40");
  REQUIRE(parser.Evaluate("20+10*2-10") == "30");
  REQUIRE(parser.Evaluate("20+10*2/10") == "22");
  REQUIRE(parser.Evaluate("(1+1)*2+2") == "6");
  REQUIRE(parser.Evaluate("(1+1)*(2+2)") == "8");

  // From function description
  REQUIRE(parser.Evaluate("Hund ~ Hündin = {no_match}") == "1"); // read as: "Hund ~ Hündin == no-match
  REQUIRE(parser.Evaluate("Hund ~ hund = {direct}") == "1"); // read as: "Hund ~ hund == direct-match
  REQUIRE(parser.Evaluate("Hund ~ Hunde = {starts_with}") == "1"); // read as: "Hund ~ hunde == starts-with-match
  REQUIRE(parser.Evaluate("Hund ~ JahrHUNDert = {contains}") == "1"); // read as: "Hund ~ JahrHUNDert == contains-match
  REQUIRE(parser.Evaluate("Mimesis ~ Mimisis = {fuzzy}") == "1"); // read as: "Hund ~ JahrHUNDert == fuzzy -match
  REQUIRE(parser.Evaluate("book:[bottle; lighter; book]") == "1");
  REQUIRE(parser.Evaluate("tabako:[bottle; lighter; book]") == "0");
  REQUIRE(parser.Evaluate("[tabako|lighter]:[bottle; lighter; book]") == "1");
  REQUIRE(parser.Evaluate("tobako~:[Bottle; Lighter; Tabako; Book]") == "[4]");
  // => read as: "tobako~:[Bottle; Lighter; Tabako; Book] == [fuzzy-match]
  REQUIRE(parser.Evaluate("tobako~:[Bottle; Lighter; Tabako; Book] = [{fuzzy}]") == "1");
  REQUIRE(parser.Evaluate("{fuzzy} : (tobako~:[Bottle; Lighter; Tabako; Book])") == "1");
  // => read as: "fuzzy-match : (tobako~:[Bottle; Lighter; Tabako; Book])
  
  // whitespace handling
  REQUIRE(parser.Evaluate(" 10 + 10") == "20");
  REQUIRE(parser.Evaluate("10 + 10") == "20");
  REQUIRE(parser.Evaluate("10+ 10") == "20");
  REQUIRE(parser.Evaluate("10 +10") == "20");
  REQUIRE(parser.Evaluate("10+10") == "20");
  REQUIRE(parser.Evaluate("Ha lo=Halo") == "0");
  REQUIRE(parser.Evaluate("Ha lo=Ha lo") == "1");
  REQUIRE(parser.Evaluate("Ha lo = Ha lo") == "1");
  REQUIRE(parser.Evaluate("10 : [ 5; ;720;10;8; 72 ;5]") == "1");
  REQUIRE(parser.Evaluate("2010 : [ 5; ;720;10;8; 72 ;5]") == "0");
  REQUIRE(parser.Evaluate("[ 10 | 11 ] : [5;720;11;8;72]") == "1");

  // math
  REQUIRE(parser.Evaluate("10 + 10") == "20");
  REQUIRE(parser.Evaluate("10 - 10") == "0");
  REQUIRE(parser.Evaluate("10 * 10") == "100");
  REQUIRE(parser.Evaluate("10 / 10") == "1");
  REQUIRE(parser.Evaluate("10 + 10 + 10") == "30");
  REQUIRE(parser.Evaluate("10 - 10 + 10") == "10");
  REQUIRE(parser.Evaluate("10 * 10 - 10 + 5") == "95");
  REQUIRE(parser.Evaluate("10 / 10 - 10+5 * 5 ") == "16");
  REQUIRE(parser.Evaluate("10 / 5 * 2 ") == "4");  

  // comparision
  REQUIRE(parser.Evaluate("10 = 10") == "1");
  REQUIRE(parser.Evaluate("10 = 11") == "0");
  REQUIRE(parser.Evaluate("10 ~ 100") == std::to_string(fuzzy::FuzzyMatch::STARTS_WITH));
  REQUIRE(parser.Evaluate("10 ~ 110") == std::to_string(fuzzy::FuzzyMatch::CONTAINS));

  REQUIRE(parser.Evaluate("hündin ~ jahrhundert = {no_match}") == "1");
  REQUIRE(parser.Evaluate("Hund ~ hund = {direct}") == "1");
  REQUIRE(parser.Evaluate("hund ~ hunde = {starts_with}") == "1");
  REQUIRE(parser.Evaluate("hund ~ jahrhundert = {contains}") == "1");
  REQUIRE(parser.Evaluate("Mimesis ~ mimisis = {fuzzy}") == "1");

  REQUIRE(parser.Evaluate("Hund ~ hund") == std::to_string(fuzzy::FuzzyMatch::DIRECT));
  REQUIRE(parser.Evaluate("hund ~ Hund") == std::to_string(fuzzy::FuzzyMatch::DIRECT));
  REQUIRE(parser.Evaluate("hund ~ hunde") == std::to_string(fuzzy::FuzzyMatch::STARTS_WITH));
  REQUIRE(parser.Evaluate("hunde ~ hund") == std::to_string(fuzzy::FuzzyMatch::NO_MATCH));
  REQUIRE(parser.Evaluate("hund ~ jahrhundert") == std::to_string(fuzzy::FuzzyMatch::CONTAINS));
  REQUIRE(parser.Evaluate("hündin ~ jahrhundert") == std::to_string(fuzzy::FuzzyMatch::NO_MATCH));
  REQUIRE(parser.Evaluate("Mimesis ~ mimisis") == std::to_string(fuzzy::FuzzyMatch::FUZZY));
  REQUIRE(parser.Evaluate("10 : [10]") == "1");
  REQUIRE(parser.Evaluate("10 : [10;5]") == "1");
  REQUIRE(parser.Evaluate("10 : [5;10]") == "1");
  REQUIRE(parser.Evaluate("10 : [5;720;10;8;72]") == "1");
  REQUIRE(parser.Evaluate("10 : [5;720;11;8;72]") == "0");
  REQUIRE(parser.Evaluate("[10|11] : [5;720;11;8;72]") == "1");
  REQUIRE(parser.Evaluate("10 < 100") == "1");
  REQUIRE(parser.Evaluate("10 > 100") == "0");
  REQUIRE(parser.Evaluate("10 > 9") == "1");
  REQUIRE(parser.Evaluate("10 < 9") == "0");
  REQUIRE(parser.Evaluate("10 < 9") == "0");
  REQUIRE(parser.Evaluate("10 >= 9") == "1");
  REQUIRE(parser.Evaluate("10 >= 10") == "1");
  REQUIRE(parser.Evaluate("10 >= 11") == "0");
  REQUIRE(parser.Evaluate("10 <= 11") == "1");
  REQUIRE(parser.Evaluate("10 <= 10") == "1");
  REQUIRE(parser.Evaluate("10 <= 9") == "0");
  std::string res = parser.Evaluate("mimisis ~: [Eingedenken; das Hinzutretende; Mimesis; Leid]");
  REQUIRE(res == "[" + std::to_string(fuzzy::FuzzyMatch::FUZZY) + "]");
  REQUIRE(parser.Evaluate(std::to_string(fuzzy::FuzzyMatch::FUZZY) + ":"+res) == "1"); // 4:[...]
  res = parser.Evaluate("Mimesis ~: [Eingedenken; mimesisch; das Hinzutretende; Leid]");
  REQUIRE(res == "[" + std::to_string(fuzzy::FuzzyMatch::STARTS_WITH) + "]");

  // Does not work, requires brackets!
  REQUIRE(parser.Evaluate("mimisis ~: [Eingedenken; das Hinzutretende; Leid]") == "[]");

  REQUIRE(parser.Evaluate("Hund ~ hund = " + std::to_string(fuzzy::FuzzyMatch::DIRECT)) == "1");
  REQUIRE(parser.Evaluate("Mimesis ~ mimisis =" + std::to_string(fuzzy::FuzzyMatch::FUZZY)) == "1");
  REQUIRE(parser.Evaluate("hündin ~ jahrhundert =" + std::to_string(fuzzy::FuzzyMatch::NO_MATCH)) == "1");
  REQUIRE(parser.Evaluate("hündin ~ jahrhundert =" + std::to_string(fuzzy::FuzzyMatch::FUZZY)) == "0");

  // logical 
  REQUIRE(parser.Evaluate("1 && 1") == "1");
  REQUIRE(parser.Evaluate("1 && 0") == "0");
  REQUIRE(parser.Evaluate("0 && 1") == "0");
  REQUIRE(parser.Evaluate("0 && 0") == "0");
  REQUIRE(parser.Evaluate("1 || 1") == "1");
  REQUIRE(parser.Evaluate("1 || 0") == "1");
  REQUIRE(parser.Evaluate("0 || 1") == "1");
  REQUIRE(parser.Evaluate("0 || 0") == "0");
  REQUIRE(parser.Evaluate("(20*10>4) && (4:[10;3; 4])") == "1");

  // brackets 
  REQUIRE(parser.Evaluate("4 - ( 2 + 2)") == "0");
  REQUIRE(parser.Evaluate("4 - ( 2 + 2) + 2") == "2");
  REQUIRE(parser.Evaluate("((1+1)*2+2)*(2+2)+10") == "34");
  REQUIRE(parser.Evaluate("((1+1)*2+2)*(2+2)+(10*(2+(10-10)))") == "44");

  // Execution order 
  REQUIRE(parser.Evaluate("2+4*2") == "10");
  REQUIRE(parser.Evaluate("(10-10)*2+2+4*2") == "10");
}


TEST_CASE("Text replacements", "[parser]") {
  std::map<std::string, std::string> substitutes = {{"player_name", "fux"}, {"inventory", "[book; tabako; wine]"}};
  ExpressionParser parser(&substitutes);

  // name reduction
  REQUIRE(parser.Evaluate("{player_name}=fux") == "1");
  REQUIRE(parser.Evaluate("{player_name}=jan") == "0");
  REQUIRE(parser.Evaluate("{player_name}={player_name}") == "1");
  REQUIRE(parser.Evaluate("{player_name}=player_name") == "0");

  // list reduction
  REQUIRE(parser.Evaluate("tabako:{inventory}") == "1");
  REQUIRE(parser.Evaluate("[{fuzzy}] : (tobako~:{inventory})") == "1");
  REQUIRE(parser.Evaluate("cigarettes:{inventory}") == "0");
}
