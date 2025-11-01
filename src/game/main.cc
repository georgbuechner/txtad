#include "game/game/game.h"
#include "game/server/websocket_server.h"
#include "game/utils/defines.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <filesystem>
#include <httplib.h> 
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

int main() {

  util::SetUpLogger(txtad::FILES_PATH, txtad::LOGGER, spdlog::level::debug);
  util::LoggerContext scope(txtad::LOGGER);

  // Create web socket server 
  std::shared_ptr<WebsocketServer> wss = std::make_shared<WebsocketServer>();

  // Create games
  auto games = parser::InitGames(txtad::GAMES_PATH);

  Game::set_msg_fn([&wss](const std::string& id, const std::string& msg) {
    util::Logger()->debug("MAIN: SendMessage: {}, {}", id, msg);
    try {
      wss->SendMessage(id, msg);
    } catch(std::exception& e) {
      util::Logger()->warn("MAIN: SendMessage failed: {}", e.what());
    }
  });

  WebsocketServer::set_handle_event([&games](const std::string& id, const std::string& game, const std::string& event) {
    util::Logger()->debug("MAIN: Handling: {}, {}", game, event);
    if (games.count(game) > 0) {
      try {
        games.at(game)->HandleEvent(id, event);
      } catch(std::exception& e) {
        util::Logger()->warn("MAIN: Handling: {}, {} failed: {}", game, event, e.what());
      }
    }
  });

  std::thread thread_http([&games]() {
    httplib::Server http_server;
    for (const auto& game : games) {
      http_server.set_mount_point("/" + game.second->name(), game.second->path() + "/" + txtad::HTML_PATH);
      http_server.set_mount_point("/" + game.second->name(), txtad::FILES_PATH + "/" + txtad::HTML_PATH);
    }
    util::Logger()->info("MAIN: Successfully started http-server on port 4080");
    http_server.listen("0.0.0.0", 4080);
  });

  std::thread thread_wss([&games, &wss]() {
    wss->Start(4181);
  });

  thread_http.join();
}
