#include "utils.h"
#include "cleanup_dtor.h"
#include <exception>
#include <fmt/core.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <random>
#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sstream>
#include <openssl/evp.h>
#include <openssl/sha.h>


std::string util::LOGGER = "---";

void util::SetUpLogger(const std::string& main_path, const std::string& name, spdlog::level::level_enum log_level) {
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_st>());
  sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(main_path + "/logs/" + name + "_logfile.txt", 21, 15));
  auto logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
  spdlog::register_logger(logger);
  spdlog::flush_every(std::chrono::seconds(5));
  spdlog::flush_on(spdlog::level::err);
  // set log-level
  logger->set_level(log_level);
}

std::shared_ptr<spdlog::logger> util::Logger() {
  return spdlog::get(LOGGER);
}

struct CommaIterator : public std::iterator<std::output_iterator_tag, void, void, void, void> {
  std::ostream *os;
  std::string comma;
  bool first;

  CommaIterator(std::ostream& os, const std::string& comma) : os(&os), comma(comma), first(true) { }

  CommaIterator& operator++() { return *this; }
  CommaIterator& operator++(int) { return *this; }
  CommaIterator& operator*() { return *this; }
  template <class T>
  CommaIterator& operator=(const T& t) {
    if (first) {
      first = false;
    } else {
      *os << comma;
    }
    *os << t;
    return *this;
  }
};

std::string util::Join(const std::vector<std::string>& vec, const std::string& separator) {
  std::ostringstream oss; 
  std::copy(vec.begin(), vec.end(), CommaIterator(oss, separator)); 
  return oss.str();
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

std::string util::Strip(std::string str, char c) {
  for(;;) {
    if (str.front() == c) 
      str.erase(0, 1);
    else if (str.back() == c) 
      str.pop_back();
    else 
      break;
  }
  return str;
}

std::string util::ReplaceAll(std::string str, const std::string& from, const std::string& to) {
  size_t pos = 0;
  while ((pos = str.find(from, pos)) != std::string::npos) {
    str.replace(pos, from.length(), to);
    pos += to.length();
  }
  return str;
}

std::pair<int, int> util::InBrackets(const std::string& inp, int pos) {
  return {OpeningBracket(inp, pos), ClosingBracket(inp, pos)};
}


int util::ClosingBracket(const std::string& inp, int pos, char open, char close) {
  int accept = 0;
  int end = -1;
  for (int i=0; i<inp.length()-pos; i++) {
    char c = inp[pos+i];
    if (c == open) 
      accept++;
    else if (c == close && accept > 0) 
      accept--;
    else if (c == close && accept == 0) {
      end = pos+i;
      break;
    }
  }
  return end;
}

int util::OpeningBracket(const std::string& inp, int pos, char open, char close) {
  int accept = 0;
  int start = -1; 
  for (int i=0; i<=pos; i++) {
    char c = inp[pos-i];
    if (c == close) 
      accept++;
    else if (c == open && accept > 0) 
      accept--;
    else if (c == open && accept == 0) {
      start = pos-i;
      break;
    }
  }
  return start;
}

std::optional<nlohmann::json> util::LoadJsonFromDisc(const std::string& path) {
  try {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
      Logger()->error("Failed loading json from disc: input file could not be opened: {}", path);
    } else {
      auto json = nlohmann::json::parse(ifs);
      ifs.close();
      return json;
    }
  } catch (std::exception& e) {
    Logger()->error("Failed to load json ({}) from disc: {}", path, e.what());
  }
  return std::nullopt;
}

void util::WriteJsonToDisc(const std::string& path, const nlohmann::json& json) {
  std::ofstream ofs(path);
  if (!ofs.is_open()) {
    Logger()->error("Failed writing json to disc: output file could not be opened!");
  } else {
    ofs << json;
    ofs.close();
  }
}

std::optional<std::string> util::GetUserId(std::string& inp) {
  if (inp.find("0x7") == 0) {
    std::string user_id = inp.substr(0, 14);
    inp = inp.substr(14, inp.length()-14);
    return user_id;
  }
  return std::nullopt;
}

std::optional<std::map<std::string, nlohmann::json>> util::ValidateSimpleJson(std::string json_string, std::vector<std::string> keys) {
  nlohmann::json json;
  try {
    json = nlohmann::json::parse(json_string); 
  } catch (std::exception& e) {
    Logger()->warn(fmt::format("ValidateJson: Failed parsing json: {}", e.what()));
    return std::nullopt;
  }
  for (auto key : keys) {
    std::vector<std::string> depths = util::Split(key, "/");
    if (depths.size() > 0) {
      if (json.count(depths[0]) == 0) {
        Logger()->info(fmt::format("ValidateJson: Missing key: {}", key));
        return std::nullopt;
      }
    }
    if (depths.size() > 1) {
      if (json[depths[0]].count(depths[1]) == 0) {
        Logger()->info(fmt::format("ValidateJson: Missing key: {}", key));
        return std::nullopt;
      }
    }
  }
  return json;
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

std::string util::HashSha3512(const std::string& input) {
  unsigned int digest_length = SHA512_DIGEST_LENGTH;
  const EVP_MD* algorithm = EVP_sha3_512();
  uint8_t* digest = static_cast<uint8_t*>(OPENSSL_malloc(digest_length));
  CleanupDtor dtor([digest](){OPENSSL_free(digest);});

  EVP_MD_CTX* context = EVP_MD_CTX_new();
  EVP_DigestInit_ex(context, algorithm, nullptr);
  EVP_DigestUpdate(context, input.c_str(), input.size());
  EVP_DigestFinal_ex(context, digest, &digest_length);
  EVP_MD_CTX_destroy(context);

  std::stringstream stream;
  stream << std::hex;

  for (auto b : std::vector<uint8_t>(digest,digest+digest_length))
    stream << std::setw(2) << std::setfill('0') << (int)b;
  return stream.str();
}

std::string util::generate_random_hex_string(size_t length) {
  const std::string characters = "0123456789ABCDEF";
  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<> distribution(0, characters.size() - 1);

  std::string random_string;
  for (size_t i = 0; i < length; ++i) {
    random_string += characters[distribution(generator)];
  }

  return random_string;
}

std::string util::GetPage(const std::string& path) {
  // Read loginpage and send
  std::ifstream read(path);
  if (!read) {
    util::Logger()->warn(fmt::format("Wrong file passed: {}", path));
    return "";
  }
  std::string page( (std::istreambuf_iterator<char>(read) ),
                    std::istreambuf_iterator<char>()     );
  // Delete file-end marker
  page.pop_back();
  return page;
}
