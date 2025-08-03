/**
 * @author fux
 */
#ifndef SHARED_UTILS_PARSER_EXPRESSIONPARSER_H_
#define SHARED_UTILS_PARSER_EXPRESSIONPARSER_H_

#include "game/utils/defines.h"
#include <map>
#include <string>

class ExpressionParser{
  public:
    using SubstituteFN = std::function<std::string(std::string)>;
    
    /**
     * Constructor including the possibility to substitute certain strings
     * with given values (for instance you could map room to the current player
     * room by including {"room", [current_room]} to the map. Then room will 
     * be substituted by the value of [current_room]).
     * @param[in] substitute (map with possible substitutes)
     */
    ExpressionParser();
    ExpressionParser(const SubstituteFN& fn);

    /**
     * return value of logical or mathematical expression
     * Always returns a string ("0" = false and "1" = true for boolean expressions 
     * Operand: +, -, *, /, =, <, >, <=, >= 
     * Complex operands: 
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
     * fuzzy-in-list (~:) -> list of fuzzy-match results, f.e. [1,4]
     * - "tobako~:[bottle; lighter; Tabako; Book]" == "[4]" (read as: "[fuzzy-match]
     * -> can also be used like this: "[4] : (tobako~:[bottle; lighter; Tabako; Book])" == "1"
     * @param[in] input (logical/mathematical expression)
     * @return string
     */
    std::string Evaluate(std::string input) const;

  private:

    // members 
    SubstituteFN _substitute_fn;  ///< map with substitutes.
    static const std::map<std::string, std::string> _default_subsitutes;
    static std::map<std::string, std::string(*)(const std::string&, const std::string&)> _opts;

    // methods 
    std::string evaluate(std::string input) const;
    std::string StripAndSubstitute(std::string str) const;

    // static methods 

    /** 
     * Adds brackets around certain expressions (f.e. '*' and '/')
     * @param[in] inp (logical expression)
     * @return modified expression
     */
    static std::string EnsureExecutionOrder(std::string inp);

    // static methods
    /**
     * Finds operand on the right most side of expression.
     * @param[in] inp (logical expression)
     * @return operand and it's position in the given expression.
     */
    static std::pair<int, std::string> LastOpt(const std::string& inp);
};

#endif
