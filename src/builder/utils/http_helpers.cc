#include "http_helpers.h"
#include "httplib.h"
#include "shared/utils/utils.h"

std::string _http::Get(const httplib::Request& req, const std::string &field) {
  if (req.get_param_value_count(field) == 0) 
    throw _t_exception({401, "Missing field \"" + field + "\""});
  return req.get_param_value(field);
}

std::string _http::UrlPath(const std::string& url) {
  size_t query_pos = url.find("?"); 
  if (query_pos != std::string::npos) {
    return url.substr(0, query_pos);
  }
  return url;
}
std::string _http::UrlQuery(const std::string& url) {
  size_t query_pos = url.find("?"); 
  if (query_pos == std::string::npos) {
    return "";
  }
  return url.substr(query_pos+1);
}

std::string _http::Referer(const httplib::Request& req, std::string msg) {
  std::string referer = req.get_header_value("Referer");
  util::Logger()->info("Got referer: " + referer);
  if (referer.empty()) {
    return msg.empty() ? "/" : "/?msg=";
  }

  std::string path = UrlPath(referer);
  std::string query = UrlQuery(referer); 

  httplib::Params params; 
  if (!query.empty()) {
    httplib::detail::parse_query_text(query, params);
  }

  // Handle msg 
  params.erase("msg"); 

  if (!msg.empty()) {
    params.emplace("msg", msg);
  }

  std::string new_query = httplib::detail::params_to_query_str(params);

  if (new_query.empty()) {
    return path.empty() ? "/" : new_query;
  }
  return path + "?" + new_query;
}
