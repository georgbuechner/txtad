#include "http_helpers.h"
#include "httplib.h"
#include "shared/utils/utils.h"
#include <fstream>
#include <optional>
#include <stdexcept>

std::string _http::Get(const httplib::Request& req, const std::string &field) {
  if (req.get_param_value_count(field) == 0) 
    throw _t_exception({401, "Missing field \"" + field + "\""});
  return req.get_param_value(field);
}

std::optional<std::string> _http::GetField(const httplib::Request& req, const std::string& field) {
  if (req.form.has_field(field)) {
    return std::optional<std::string>(req.form.get_field(field));
  }
  return std::nullopt;
}

std::string _http::GetFileExtension(const httplib::FormData& file) {
  std::string extension = "";
  size_t dot_pos = file.filename.find_last_of('.');
  if (dot_pos != std::string::npos) {
    extension = file.filename.substr(dot_pos);
  }
  return extension;
}

void _http::SafeFile(const httplib::FormData& file, const std::string& media_path) {
  // Save the file
  std::ofstream outfile(media_path, std::ios::binary);
  if (!outfile.is_open()) {
    throw std::runtime_error("Failed creating media element. Could not save file: " + media_path);
  }
  outfile.write(file.content.c_str(), file.content.length());
  outfile.close();
}

void _http::LoadMediaFile(httplib::Response& resp, const std::string& media_path) {
  // Open and read the file
  std::ifstream file(media_path, std::ios::binary);
  if (!file.is_open()) {
    resp.status = 500;
    resp.set_content("Could not open media file: " + media_path, "text/plain");
    return;
  }
  
  // Get file size
  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);
  
  // Read file content
  std::string content(file_size, '\0');
  file.read(&content[0], file_size);
  file.close();

  // Determine content type from file extension
  std::string extension = media_path.substr(media_path.find_last_of('.') + 1);
  std::string content_type = util::GetContentType(extension);
  
  // Set response headers
  resp.set_header("Content-Type", content_type);
  resp.set_header("Content-Length", std::to_string(file_size));
  // resp.set_header("Cache-Control", "public, max-age=86400");  // Cache for 1 day
  
  // Send the file content
  resp.set_content(content, content_type);
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

