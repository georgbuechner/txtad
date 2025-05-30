#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <list>
#include <map>
#include <algorithm>

namespace fuzzy
{
    enum FuzzyMatch {
      NO_MATCH = 0,
      DIRECT,
      STARTS_WITH,
      CONTAINS,
      FUZZY 
    };

    /**
    * Levenshtein distance algorithm as a iterative function
    * @parameter const char* (searched word)
    * @parameter const char* (target word)
    * @return int (levenshtein distance)
    **/
    size_t levenshteinDistance(const char* chS, const char* chT);
            
    double fast_search(const char* chS, const char* chT, size_t lenS, size_t lenIn);

    /**
    * @brief compare to words with fuzzy search and case insensetive, AND modify id
    * @parameter sWord1 (searched word)
    * @parameter sWord2 (word)
    * @param[out] ld indicating levenstein (-1 if false, 1 if exact-, 2 if contains-match, 0-fuzzynes)
    * @return bool 
    */
    double fuzzy_cmp(std::string sWord1, std::string sWord2);

    FuzzyMatch fuzzy(std::string str1, std::string str2);
    
    /**
    * @param[in, out] str string to be modified
    */
    void convertToLower(std::string &str);
}

#ifdef __DEFINE_ONE__

#endif
