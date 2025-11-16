#include "http_helpers.h"
#include "shared/utils/utils.h"

std::string _http::Get(const httplib::Request& req, const std::string &field) {
  if (req.get_param_value_count(field) == 0) 
    throw _t_exception({401, "Missing field \"" + field + "\""});
  return req.get_param_value(field);
}

std::string _http::Referer(const httplib::Request& req) {
  std::string referer = req.get_header_value("Referer");
  util::Logger()->info("Got referer: " + referer);
  referer.replace(0, referer.find("/", 10), "");
  util::Logger()->info("Got referer: " + referer);
  auto pos = referer.find("?");
  if (pos != std::string::npos) {
    referer = referer.substr(0, pos);
    util::Logger()->info("Got referer: " + referer);
  }
  return referer;
}
