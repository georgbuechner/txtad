#include <iostream>
#include <httplib.h> 
#include <thread>
#include <vector>

int main() {

  std::vector<std::string> games = {"test"};

  std::thread thread_http([&games]() {
    httplib::Server http_server;
    std::cout << "Starting server" << std::endl;
    auto ret = http_server.set_mount_point("/", "web/");
    for (const auto& game : games) {
      auto ret = http_server.set_mount_point("/" + game, "../data/games/" + game + "/web/");
    }
    std::cout << "Successfully started http-server on port 4080" << std::endl;
    http_server.listen("0.0.0.0", 4080);
  });

  thread_http.join();
}
