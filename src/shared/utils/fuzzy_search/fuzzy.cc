#include "fuzzy.h"

/**
* @brief Levenshteindistance algorithm as a iterative function
* @parameter const char* (search string)
* @parameter const char* (target string)
* @return int (levenshteindistance)
**/
size_t fuzzy::levenshteinDistance(const char* chS, const char* chT)
{
  size_t len_S = strlen(chS)+1;
  size_t len_T = strlen(chT)+1;
  size_t* d = new size_t[len_S*len_T];
  int substitutionCost = 0;

  std::memset(d, 0, sizeof(size_t) * len_S * len_T);

  for(size_t j=0; j<len_T; j++)
    d[j] = j;
  for(size_t i=0; i<len_S; i++)
    d[i*len_T] = i;

  for(size_t j=1, jm=0; j<len_T; j++, jm++) {
    for(size_t i=1, im=0; i<len_S; i++, im++) {
      if(chS[im] == chT[jm])
        substitutionCost = 0;
      else
        substitutionCost = 1;

      d[i*len_T+j] = std::min (   d[(i-1) * len_T+j    ] + 1,
                     std::min (   d[i     * len_T+(j-1)] + 1,
                                  d[(i-1) * len_T+(j-1)] + substitutionCost));
    }
  }
   
  size_t score = d[len_S*len_T-1];
  delete []d;
  return score; 
}


double fuzzy::fast_search(const char* chS, const char* chIn, size_t lenS, size_t lenIn) {
  double score = 0;
  bool r = true;
  if (lenS > lenIn) return 1;
  if (lenIn > lenS) score = 0.1;

  for(size_t i=0; i<strlen(chIn); i++) {
    r = true;
    for(size_t j=0; j<strlen(chS); j++) {
      if(chIn[i+j] != chS[j]) {
        r = false;
        break;
      }
    }
    if (i==0 && r == true) 
      return score;
    else if (r==true) 
      return 0.19;
  }
  return 1;
}

/**
* @brief compare to words with fuzzy search and case insensetive, AND modify id
* @parameter sWord1 (searched word)
* @parameter sWord2 (word)
* @param[out] ld indicating levenstein (-1 if false, 0 if exact-, 2 if contains-, 1-2 if fuzzy-match)
* @return bool 
*/
double fuzzy::fuzzy_cmp(std::string sWord1, std::string sWord2) {
  //Convert both string to lowercase
  convertToLower(sWord1);
  convertToLower(sWord2);

  //Check lengths
  double len1 = sWord1.length();
  double len2 = sWord2.length();

  //Fast search (full match: 0, beginswith: 0.1, contains: 0.19)
  double fast = fast_search(sWord2.c_str(), sWord1.c_str(), len2, len1); 
  if(fast < 1) return fast;

  //Check whether length of words are to far appart.
  if(len1>len2 && len2/len1 < 0.8)      
      return 1;
  else if(len1/len2 < 0.8) 
      return 1; 

  //Calculate levenshtein distance
  size_t distance = levenshteinDistance(sWord1.c_str(), sWord2.c_str());

  //Calculate score
  return static_cast<double>(distance)/ len2;
}

/**
* @param[in, out] str string to be modified
*/
void fuzzy::convertToLower(std::string &str) {
  std::locale loc1;
  for(unsigned int i=0; i<str.length(); i++)
      str[i] = tolower(str[i], loc1);
}

fuzzy::FuzzyMatch fuzzy::fuzzy(std::string str1, std::string str2) {
  double fuzzy_res = fuzzy::fuzzy_cmp(str1, str2); 
  if (fuzzy_res == 0)
    return FuzzyMatch::DIRECT;
  if (fuzzy_res == 0.1)
    return FuzzyMatch::STARTS_WITH;
  if (fuzzy_res == 0.19) 
    return FuzzyMatch::CONTAINS;
  else if ((str1.length() <= 4 && fuzzy_res < 0.2) || (str1.length() > 4 && fuzzy_res <= 0.3))
    return FuzzyMatch::FUZZY;
  return FuzzyMatch::NO_MATCH;
}
