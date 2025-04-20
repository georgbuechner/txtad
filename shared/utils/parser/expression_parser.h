/**
 * @author fux
 */
#ifndef SRC_TEXTADVENTURE_TOOLS_LOGICPARSER_H_
#define SRC_TEXTADVENTURE_TOOLS_LOGICPARSER_H_

#include <map>
#include <string>
#include <vector>

class ExpressionParser{
  public:
    /**
     * Constructor stetting up operands and simple operands.
     */
    ExpressionParser();

    /**
     * Contructor including the possibility to substitute certain strings
     * with given values (for instance you could map room to the current player
     * room by including {"room", [current_room]} to the map. Then room will 
     * be substituted by the value of [current_room]).
     * @param[in] substitute (map with possible substitutes)
     */
    ExpressionParser(std::map<std::string, std::string> substitute);

    /**
     * return value of logical or mathamtical expression
     * Always retruns a string ("0" = false and "1" = true for boolean expressions 
     * Operand: +, -, *, /, =, <, >, <=, >= 
     * Compex operands: 
     * fuzzy-search (~) -> 0|1|2|3|4
     * - "Hund ~ Hündin" = "0" (no-match)
     * - "Hund ~ hund" = "1" (Direct-match)
     * - "Hund ~ Hunde" = "2" (starts-with-match)
     * - "Hund ~ JahrHUNDert" = "3" (contains-match)
     * - "Mimesis ~ Mimisis" = "4" (fuzzy-match)
     * in-list (:) -> 0|1
     * - "book:[bottle; lighter; book]" == "1"
     * - "tabako:[bottle; lighter; book]" == "0"
     * - "[tabako|lighter]:[bottle; lighter; book]" == "1"
     * fuzzy-in-list (~:) -> list of fuzzy-match restults, f.e. [1,4]
     * - "tobako~:[bottle; lighter; Tabako; Book]" == "[4]" (read as: "[fuzzy-match]
     * -> can also be used like this: "[4] : (tobako~:[bottle; lighter; Tabako; Book])" == "1"
     * @param[in] input
     * @return string
     */
    std::string evaluate(std::string input, bool no_brackets=false);

    std::string EnsureExecutionOrder(std::string inp);

  private:

    // members 
    std::map<std::string, std::string> substitue_;  ///< map with substitutes.
    static std::map<std::string, std::string(*)(const std::string&, const std::string&)> opts_;

    // methods 

    // static methods
    static std::pair<int, std::string> LastOpt(const std::string& inp);
    static std::pair<int, int> InBrackets(const std::string& inp, int pos);
};

#endif
