#include "game/game/game.h"
#include "game/server/websocket_server.h"
#include "game/utils/defines.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <filesystem>
#include "httplib.h"
#include <memory>
#include <mutex>
#include <nlohmann/json_fwd.hpp>
#include <shared_mutex>
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
  for (auto& [_, game]: games) {
    game->set_running(true);
  }

  Game::set_msg_fn([&wss](const std::string& id, const std::string& msg) {
    util::Logger()->debug("MAIN: SendMessage: {}, {}", id, msg);
    try {
      wss->SendMessage(id, msg);
    } catch(std::exception& e) {
      util::Logger()->warn("MAIN: SendMessage failed: {}", e.what());
    }
  });

  std::shared_mutex mtx;

  WebsocketServer::set_handle_event([&games, &mtx](const std::string& id, const std::string& game, const std::string& event) {
    util::Logger()->debug("MAIN: Handling: {}, {}", game, event);
    std::shared_lock sl(mtx);
    if (games.contains(game) > 0) {
      try {
        games.at(game)->HandleEvent(id, event);
      } catch(std::exception& e) {
        util::Logger()->warn("MAIN: Handling: {}, {} failed: {}", game, event, e.what());
      }
    }
  });

  std::thread thread_http([&games, &mtx]() {
    httplib::Server http_server;
    for (const auto& game : games) {
      http_server.set_mount_point("/" + game.second->name(), game.second->path() + "/" + txtad::HTML_PATH);
      http_server.set_mount_point("/" + game.second->name(), txtad::FILES_PATH + "/" + txtad::HTML_PATH);
    }

    http_server.Get("/api/games/running", [&](const httplib::Request& req, httplib::Response& resp) {
        std::shared_lock sl(mtx);
        std::map<std::string, bool> game_info;
        for (const auto& [id, game] : games) {
          game_info[id] = game->running();
        }
        resp.status = 200;
        resp.set_content(nlohmann::json(game_info).dump(), "application/json");
    });

    http_server.Get("/api/game/reload/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
        std::unique_lock sl(mtx);
        std::string game_id = req.path_params.at("game_id");
        std::string game_path = txtad::GAMES_PATH + game_id;
        if (std::filesystem::is_directory(game_path)) {
          games.erase(game_id);
          games[game_id] = std::make_shared<Game>(game_path, game_id);
          games[game_id]->set_running(true);
          resp.status = 200;
        } else {
          resp.status = 400;
        }
    });

    http_server.Get("/api/game/stop/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
        std::unique_lock sl(mtx);
        std::string game_id = req.path_params.at("game_id");
        if (games.contains(game_id)) {
          games.erase(game_id);
          resp.status = 200;
        } else {
          resp.status = 400;
        }
    });

    util::Logger()->info("MAIN: Successfully started http-server on port 4080");
    http_server.listen("0.0.0.0", 4080);
  });

  std::thread thread_wss([&games, &wss]() {
    wss->Start(4181);
  });

  thread_http.join();
}
