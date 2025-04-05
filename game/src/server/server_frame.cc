#include <httplib.h>
#include <memory>
#include <iostream>
#include <string>
#include "server_frame.h"

#define CORS_ALLOWED_ORIGIN "http://gc-android"
#define UI_PATH "web/"
#define COOKIE_NAME "SESSID"

using namespace httplib;

std::string GetCookie(const Request &req);

ServerFrame::ServerFrame(std::string path_to_cert, std::string path_to_key) 
#ifdef _DEVELOPMENT_
  server_(path_to_cert.c_str(), path_to_key.c_str()) 
#endif
{ 
  std::cout << "Created server." << std::endl;
}

int ServerFrame::Start(int port) {
  auto ret = server_.set_mount_point("/", UI_PATH);
  if (!ret) {
    std::cout << "The specified base directory does not exist" << std::endl;

  }

  // Add handlers for battlefield
  server_.Get("/api/heartbeat", [&](const Request& req, Response& resp) { resp.status = 200; });

  std::cout << "Successfully started http-server on port: " << port << std::endl;
  server_.set_keep_alive_max_count(1);
  return server_.listen("0.0.0.0", port);
}

//Handler

bool ServerFrame::IsRunning() {
  return server_.is_running();
}

void ServerFrame::Stop() {
  server_.stop();
}


ServerFrame::~ServerFrame() {
  Stop();
}

