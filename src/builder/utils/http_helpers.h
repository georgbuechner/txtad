#ifndef SRC_BUILDER_UTILS_HTTP_HELPERS_H
#define SRC_BUILDER_UTILS_HTTP_HELPERS_H

#include <string>
#include "httplib.h"
#include <utility>

namespace _http {
  typedef std::pair<int, std::string> _t_exception;
  std::string Get(const httplib::Request& req, const std::string& field);

  std::string Referer(const httplib::Request& req, std::string msg="");

  std::string UrlPath(const std::string& url);
  std::string UrlQuery(const std::string& url);
}

#endif
