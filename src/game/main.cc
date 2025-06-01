#include "game/game/game.h"
#include "game/server/websocket_server.h"
#include "game/utils/defines.h"
#include "shared/utils/utils.h"
#include <filesystem>
#include <httplib.h> 
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

std::map<std::string, std::shared_ptr<Game>> InitGames() {
  std::map<std::string, std::shared_ptr<Game>> games;
  for (const auto& dir : std::filesystem::directory_iterator(txtad::GAMES_PATH)) {
    const std::string filename = dir.path().filename();
    games[filename] = std::make_shared<Game>(dir.path(), filename);
    util::Logger()->info("MAIN:InitGames: Created game *{}* @{}", filename, dir.path().string());
  }
  return games;
}

int main() {

  util::SetUpLogger(txtad::FILES_PATH, txtad::LOGGER, spdlog::level::info);
  util::LoggerContext scope(txtad::LOGGER);

  // Create web socket server 
  std::shared_ptr<WebsocketServer> wss = std::make_shared<WebsocketServer>();

  // Create games
  auto games = InitGames();

  Game::set_msg_fn([&wss](const std::string& id, const std::string& msg) {
    util::Logger()->debug("MAIN: SendMessage: {}, {}", id, msg);
    wss->SendMessage(id, msg);
  });

  WebsocketServer::set_handle_event([&games](const std::string& id, const std::string& game, const std::string& event) {
    util::Logger()->debug("MAIN: Handling: {}, {}", game, event);
    if (games.count(game) > 0) {
      games.at(game)->HandleEvent(id, event);
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
