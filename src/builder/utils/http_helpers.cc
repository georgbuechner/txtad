#include "http_helpers.h"

std::string _http::Get(const httplib::Request& req, const std::string &field) {
  if (req.get_param_value_count(field) == 0) 
    throw _t_exception({401, "Missing field \"" + field + "\""});
  return req.get_param_value(field);
}

std::string _http::Referer(const httplib::Request& req) {
  std::string referer = req.get_header_value("Referer");
  referer.replace(0, referer.find("/", 10), "");
  return referer;
}
