#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include "utils/utils.h"
#include "utils/defines.h"
#include <nlohmann/json.hpp>

#define LOGGER "logger"

std::string util::ReturnToLower(std::string &str) {
  std::string str2;
  for (unsigned int i=0; i<str.length(); i++)
    str2 += tolower(str[i]);
  return str2;
}

std::string util::GetPage(std::string path) {
  // Read loginpage and send
  std::ifstream read(path);
  if (!read) {
    std::cout << "Wrong file passed: " << path << std::endl;
    return "";
  }
  std::string page( (std::istreambuf_iterator<char>(read) ),
                    std::istreambuf_iterator<char>()     );
  // Delete file-end marker
  if (page.size() > 0)
    page.pop_back();
  return page;
}

std::string util::GetImage(std::string path) {
  std::ifstream f(path, std::ios::in|std::ios::binary|std::ios::ate);
  if (!f.is_open()) {
    std::cout << "file could not be found! " << path << std::endl;
    return "";
  }
  else {
    FILE* file_stream = fopen(path.c_str(), "rb");
    std::vector<char> buffer;
    fseek(file_stream, 0, SEEK_END);
    long length = ftell(file_stream);
    rewind(file_stream);
    buffer.resize(length);
    length = fread(&buffer[0], 1, length, file_stream);
    std::string s(buffer.begin(), buffer.end());
    return s;
  }
}

nlohmann::json util::LoadJsonFromDisc(std::string path) {
  nlohmann::json json;
  std::ifstream read(path);
  if (!read) {
    return "";
  }
  try {
   read >> json;
   return json;
  } 
  catch (std::exception& e) {
    read.close();
    return "";
  }
  read.close();
  return "";
}

std::vector<std::string> util::Split(std::string str, std::string delimiter) {
  std::vector<std::string> v_strs;
  size_t pos=0;
  while ((pos = str.find(delimiter)) != std::string::npos) {
    if(pos!=0)
      v_strs.push_back(str.substr(0, pos));
    str.erase(0, pos + delimiter.length());
  }
  v_strs.push_back(str);
  return v_strs;
}

std::string util::CreateId() {
  std::string id;
  static const char num[] = "0123456789";
  std::random_device dev;
  // Create random alphanumeric cookie
  for(int i = 0; i < 8; i++)
    id += num[dev()%(sizeof(num)-1)];
  return id;
}

int util::Ran(int end, int start) {
  if (end-start <= 0)
    return start;
  std::random_device dev;
  return dev()%(end-start)+start;
}

std::string util::CreateRandomString(int len) {
  std::string str;
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "_";
  std::random_device dev;
  // Create random alphanumeric cookie
  for(int i = 0; i < len; i++)
    str += alphanum[dev()%(sizeof(alphanum)-1)];
  return str;

}
