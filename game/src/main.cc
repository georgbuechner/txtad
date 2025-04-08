#include "game/game.h"
#include "server/websocket_server.h"
#include "utils/defines.h"
#include "utils/utils.h"
#include <filesystem>
#include <httplib.h> 
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

#define HTML_PATH "web/"
#define GAMES_PATH "../data/games/"

std::map<std::string, std::shared_ptr<Game>> InitGames(std::shared_ptr<WebsocketServer> wss) {
  std::map<std::string, std::shared_ptr<Game>> games;
  for (const auto& dir : std::filesystem::directory_iterator(GAMES_PATH)) {
    const std::string filename = dir.path().filename();
    games[filename] = std::make_shared<Game>(dir.path(), filename);
    spdlog::get(txtad::LOGGER)->info("MAIN:InitGames: Created game *{}*", filename);
  }
  return games;
}

int main() {

  util::SetUpLogger(txtad::LOGGER, spdlog::level::info);

  // Create websocket-server 
  std::shared_ptr<WebsocketServer> wss = std::make_shared<WebsocketServer>();

  // Create games
  auto games = InitGames(wss);

  Game::set_msg_fn([&wss](const std::string& id, const std::string& msg) {
    spdlog::get(txtad::LOGGER)->debug("MAIN: SendMessage: {}, {}", id, msg);
    wss->SendMessage(id, msg);
  });

  WebsocketServer::set_handle_event([&games](const std::string& id, const std::string& game, const std::string& event) {
    spdlog::get(txtad::LOGGER)->debug("MAIN: Handling: {}, {}", game, event);
    if (games.count(game) > 0) {
      games.at(game)->HandleEvent(id, event);
    }
  });

  std::thread thread_http([&games]() {
    httplib::Server http_server;
    for (const auto& game : games) {
      http_server.set_mount_point("/" + game.second->name(), game.second->path() + "/" + HTML_PATH);
      http_server.set_mount_point("/" + game.second->name(), HTML_PATH);
    }
    spdlog::get(txtad::LOGGER)->info("MAIN: Successfully started http-server on port 4080");
    http_server.listen("0.0.0.0", 4080);
  });

  std::thread thread_wss([&games, &wss]() {
    wss->Start(4181);
  });

  thread_http.join();
}
