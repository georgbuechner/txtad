/**
 * @author: fux
 */
#ifndef UTILS_UTIL_H
#define UTILS_UTIL_H

#include "utils/defines.h"
#include <iostream>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace util
{
  /**
  * @param[in] str
  */
  std::string ReturnToLower(std::string &str);

  /**
   * Load a page (html/ css/ javascript) from disc and return as string
   * @param[in] path to file
   * @return complete file as string
   * Load the login
   */
  std::string GetPage(std::string path);

  /**
   * @brief Load image from disc.
   * @param path (path to image)
   * @return 
   */
  std::string GetImage(std::string path);

  /**
   * @breif Loads JSON from disc.
   */
  nlohmann::json LoadJsonFromDisc(std::string path);

  /**
  * @param[in] str string to be splitet
  * @param[in] delimitter 
  * @return vector
  */
  std::vector<std::string> Split(std::string str, std::string delimiter);

  /**
   * Creates a unique ID for each user
   */
  std::string CreateId();

  /**
   * Creates a random string (chars: 0-9, a-z, A-Z) from given length
   * @param[in] len 
   * @return random string (chars: 0-9, a-z, A-Z)
   */
  std::string CreateRandomString(int len);

  int Ran(int end, int start=0);

  /**
   * @brief Implements a cryptographic hash function, uses the slower but better
   * SHA3 algorithm also named Keccak
   * @param input The string to hash, for example password email whatever.
   * @return The hashed string is returned, the input remains unchanged
   */
  std::string HashSha3512(const std::string& input);

  template<class T> 
  std::shared_ptr<T> GetRandomElement(std::map<int, std::shared_ptr<T>> map) {
    if (map.size() == 0) 
      return nullptr;
    auto it = map.begin();
    if (map.size() == 1) 
      return it->second;
    auto ran =  Ran(map.size()-1);
    std::advance(it, ran);
    return it->second;
  }
}

#endif
