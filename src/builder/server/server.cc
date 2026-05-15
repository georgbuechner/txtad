#include "builder/server/server.h" 
#include "builder/game/builder_game.h"
#include "builder/utils/defines.h"
#include "builder/utils/http_helpers.h"
#include "builder/utils/jinja_helpers.h"
#include "game/utils/defines.h"
#include "shared/objects/settings/settings.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/parser/test_file_parser.h"
#include "shared/utils/git_wrapper/git_wrapper.h"
#include "shared/utils/utils.h"
#include <exception>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::chrono_literals;

Builder::Builder(int port, std::string cli_address, int cli_port, const nlohmann::json& config) 
      : _cli(cli_address, cli_port), _env(builder::TEMPLATE_PATH), _txtad_addr(config.at("txtad").at("address")) {
  // init games
  _games = parser::InitGames<BuilderGame>(txtad::GamesPath());

  // init game-server client
  _cli.set_connection_timeout(0, 200000);
  _cli.set_read_timeout(2,0);
  _cli.set_write_timeout(2,0);
}

// methods 
void Builder::Start() {
  _srv.set_mount_point("/static", builder::STATIC_PATH);
  _srv.set_mount_point("/media", builder::MEDIA_PATH);

  // exception handling 
  _srv.set_exception_handler([](const auto& req, auto& resp, std::exception_ptr ep) {
    try {
      std::rethrow_exception(ep);
    } catch (std::exception &e) {
      util::Logger()->error(fmt::format("path: {}, error: {}", req.path, e.what()));
      resp.set_redirect(_http::Referer(req, e.what()), 303);
    } catch (...) { // See the following NOTE
      util::Logger()->error("unkwon Error (500)");
      resp.set_redirect("/?msg=Unkwon Error (500)", 303);
    }
  });

  // API 
  _srv.Post("/api/creator/register", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiRegister(req, resp); });

  _srv.Post("/api/creator/login", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiLogin(req, resp); });

  _srv.Get("/api/creator/logout", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiLogout(req, resp); });

  _srv.Post("/api/creator/requests/add", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiAddRequest(req, resp); });

  _srv.Post("/api/creator/requests/accept", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiAcceptRequest(req, resp); });

  _srv.Post("/api/creator/requests/deny", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiDenyRequest(req, resp); });

  _srv.Post("/api/:game_id/hidden/add", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiHidePath(req, resp); });

  _srv.Post("/api/:game_id/hidden/remove", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiUnhidePath(req, resp); });

  _srv.Get("/api/game/reload/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiReloadGame(req, resp); });

  _srv.Get("/api/game/stop/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiStopGame(req, resp); });

  _srv.Get("/api/tests/run/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiRunGame(req, resp); });

  _srv.Get("/api/data/ctx-ids/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiContextIDs(req, resp); });

  _srv.Get("/api/data/text-ids/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiTextIDs(req, resp); });

  _srv.Get("/api/data/ids/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiIDs(req, resp); });

  _srv.Get("/api/data/types/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiTypes(req, resp); });

  _srv.Get("/api/data/ctx-types/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiContextTypes(req, resp); });

  _srv.Get("/api/data/ctx-attributes/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiContextAttributes(req, resp); });

  _srv.Get("/api/data/type-attributes/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiTypesAttributes(req, resp); });

  _srv.Get("/api/data/media-audios/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiMediaAudios(req, resp); });

  _srv.Get("/api/data/custom-handlers/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiCustomHandlers(req, resp); });

  _srv.Get("/api/ctx/references/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiCtxReferences(req, resp); });

  _srv.Get("/api/txt/references/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiTxtReferences(req, resp); });

  _srv.Get("/api/dir/references/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiDirReferences(req, resp); });

  _srv.Get("/api/media/references/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiMediaReferences(req, resp); });

  _srv.Get("/api/archive/commit/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiCommitMessage(req, resp); });

  // PAGES 
  _srv.Get("/", [&](const httplib::Request& req, httplib::Response& resp) {
    LoadTemplate(resp, "index.html", CreateParams(req)); });

  _srv.Get("/login", [&](const httplib::Request& req, httplib::Response& resp) {
    LoadTemplate(resp, "login.html", CreateParams(req)); });

  _srv.Get("/register", [&](const httplib::Request& req, httplib::Response& resp) {
    LoadTemplate(resp, "register.html", CreateParams(req)); });

  _srv.Get("/register", [&](const httplib::Request& req, httplib::Response& resp) {
    LoadTemplate(resp, "register.html", CreateParams(req)); });

  _srv.Get("/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      PagesGame(req, resp); });

  _srv.Get("/:game_id/media", [&](const httplib::Request& req, httplib::Response& resp) {
      LoadMedia(req, resp); });

  // GAMES
  _srv.Post("/world/create", [&](const httplib::Request& req, httplib::Response& resp) {
      CreateNewGame(req, resp); });

  _srv.Post("/world/create/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      DeleteGame(req, resp); });

  // SAVE ELEMENTS

  _srv.Post("/:game_id/save/settings", [&](const httplib::Request& req, httplib::Response& resp) {
      SaveSettings(req, resp); });

  _srv.Post("/:game_id/save/tests", [&](const httplib::Request& req, httplib::Response& resp) {
      SaveTests(req, resp); });

  _srv.Post("/:game_id/save/ctx", [&](const httplib::Request& req, httplib::Response& resp) {
      SaveCtxMeta(req, resp); });

  _srv.Post("/:game_id/save/ctx/attribute", [&](const httplib::Request& req, httplib::Response& resp) {
      SaveAttribute(req, resp); });

  _srv.Post("/:game_id/save/ctx/listener", [&](const httplib::Request& req, httplib::Response& resp) {
      SaveListener(req, resp); });

  _srv.Post("/:game_id/update/ctx/description", [&](const httplib::Request& req, httplib::Response& resp) {
      SaveDescriptionElement(req, resp, false); });

  _srv.Post("/:game_id/add/ctx/description", [&](const httplib::Request& req, httplib::Response& resp) {
      SaveDescriptionElement(req, resp, true); });

  _srv.Post("/:game_id/update/text", [&](const httplib::Request& req, httplib::Response& resp) {
      SaveTextElement(req, resp, false); });

  _srv.Post("/:game_id/add/text", [&](const httplib::Request& req, httplib::Response& resp) {
      SaveTextElement(req, resp, true); });

  _srv.Post("/:game_id/save", [&](const httplib::Request& req, httplib::Response& resp) {
      SaveGame(req, resp); });

  // NEW ELEMENTS 
  _srv.Post("/:game_id/create", [&](const httplib::Request& req, httplib::Response& resp) {
      NewElement(req, resp); });

  // REMOVES ELEMENTS
  _srv.Post("/:game_id/remove/ctx/attribute", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveAttribute(req, resp); });

  _srv.Post("/:game_id/remove/ctx/listener", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveListener(req, resp); });

  _srv.Post("/:game_id/remove/ctx/description", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveDescriptionElement(req, resp); });

  _srv.Post("/:game_id/remove/text", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveTextElement(req, resp); });

  _srv.Post("/:game_id/remove/txt", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveText(req, resp); });
  _srv.Post("/:game_id/remove/ctx", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveContext(req, resp); });
  _srv.Post("/:game_id/remove/dir", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveDirectory(req, resp); });
  _srv.Post("/:game_id/remove/media", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveDirectory(req, resp); });

  _srv.Post("/:game_id/restore", [&](const httplib::Request& req, httplib::Response& resp) {
      RestoreGame(req, resp); });

  _srv.Post("/:game_id/archive/restore", [&](const httplib::Request& req, httplib::Response& resp) {
      RestoreArchive(req, resp); });

  _srv.Post("/:game_id/archive/delete", [&](const httplib::Request& req, httplib::Response& resp) {
      DeleteArchive(req, resp); });

  _srv.Post("/:game_id/archive/rename", [&](const httplib::Request& req, httplib::Response& resp) {
      RenameArchive(req, resp); });

  util::Logger()->info("MAIN: Successfully started http-server on port 4081");
  _srv.listen("0.0.0.0", 4081);
}

void Builder::GameServerCommunication() {
  while(true) {
    std::this_thread::sleep_for(1000ms);
    std::unique_lock ul(_mtx_games);
    for (auto& [_, game] : _games) {
      game->set_running(false);
    }
    if (auto res = _cli.Get("/api/games/running")) {
      if (res->status == httplib::StatusCode::OK_200) {
        try {
          const auto& game_info = nlohmann::json::parse(res->body);
          for (const auto& [id, status] : game_info.get<std::map<std::string, bool>>()) {
            if (_games.contains(id)) {
              _games.at(id)->set_running(status);
            } else {
              util::Logger()->warn(fmt::format("/api/games/running: Game {} not found", id));
            }
          }
        } catch (std::exception& e) {
          util::Logger()->warn(fmt::format("/api/games/running: Payload invalid: {}", e.what()));
        }
      } else {
        util::Logger()->warn(fmt::format("/api/games/running: Invalid Response: {}", res->status));
      }
    } else {
      util::Logger()->warn(fmt::format("/api/games/running: No Response!"));
    }
  }
}

void Builder::LoadTemplate(httplib::Response& resp, const std::string& page, const jinja2::ValuesMap& params) {
  try {
    resp.set_content(_env.LoadTemplate(page, params), "text/html");
    resp.status = 200;
  } catch (std::exception& e) {
    resp.status = 500; 
    resp.set_content(e.what(), "text/plain"); 
  } 
}

jinja2::ValuesMap Builder::CreateParams(const httplib::Request& req, std::string game_id, std::string type, 
    const std::vector<TestCase>& test_cases) {
  std::shared_lock sl(_mtx_games);
  util::Logger()->debug(fmt::format("Builder::create_params"));
  jinja2::ValuesMap params;

  // Add general information
  params.emplace("txtad_address", _txtad_addr);
  params.emplace("games", _jinja::Map(_games));
  params.emplace("game_ids", _jinja::MapKeys(_games));
  try {
    auto creator = _manager.CreatorFromCookie(req);
    params.emplace("creatorname", creator->username());
    params.emplace("worlds", _jinja::SetToVec(creator->worlds()));
    params.emplace("shared", _jinja::SetToVec(creator->shared()));
    params.emplace("pending", _jinja::SetToVec(creator->pending()));
    params.emplace("is_owner", creator->OwnsGame(game_id));
  } catch (_http::_t_exception e) {
    params.emplace("creatorname", "");
  }
  if (req.get_param_value_count("msg") > 0) {
    params.emplace("msg", req.get_param_value("msg"));
  }
  if (req.get_param_value_count("open") > 0) {
    params.emplace("open", req.get_param_value("open"));
  }

  // Add Game Information
  if (game_id != "" && _games.contains(game_id)) {
    std::string path = (req.has_param("path")) ? util::Strip(req.get_param_value("path"), '/') : "";
    const auto& game = _games.at(game_id);
    util::Logger()->debug(fmt::format("Builder::create_params. Got path: {}", path));
    params.emplace("game", jinja2::Reflect(PtrView<BuilderGame>{game.get()}));
    params.emplace("all_paths", _jinja::Map(game->GetPaths()));
    params.emplace("selections", _jinja::Map(game->GetPaths(path)));
    if (req.has_param("type") && builder::REVERSE_FILE_TYPE_MAP.contains(type)) {
      auto t_type = builder::REVERSE_FILE_TYPE_MAP.at(type);
      if (t_type == builder::FileType::CTX) {
        params.emplace("ctx", jinja2::Reflect(PtrView<Context>{game->contexts().at(path).get()}));
      } else if (t_type == builder::FileType::TXT) {
        params.emplace("texts", jinja2::Reflect(PtrView<Text>{game->texts().at(path).get()}));
      }
    }
    params.emplace("test_cases", _jinja::Vec(test_cases));
    params.emplace("path", path);
    params.emplace("back", (path.rfind("/") == std::string::npos) ? "" : path.substr(0, path.rfind("/")));
    params.emplace("modified", _jinja::Vec(game->modified()));
    params.emplace("backup_infos", _jinja::Map(game->backup_infos()));
    params.emplace("current_branch", git::get_current_branch(game->path()));
  }
  util::Logger()->debug(fmt::format("Builder::create_params. done"));
  return params;
}

void Builder::ApiRegister(const httplib::Request& req, httplib::Response& resp) {
  const std::string msg = _manager.Register(req, resp);
  resp.set_redirect((msg != "") ? "/register?msg=" + msg : "/", 303);
}

void Builder::ApiLogin(const httplib::Request& req, httplib::Response& resp) {
  const std::string msg = _manager.Login(req, resp);
  resp.set_redirect((msg != "") ? "/login?msg=" + msg : "/", 303);
}

void Builder::ApiLogout(const httplib::Request& req, httplib::Response& resp) {
  _manager.Logout(req, resp);
}

void Builder::ApiAddRequest(const httplib::Request& req, httplib::Response& resp) {
  try {
    const std::string game_id = _http::Get(req, "game_id");
    auto creator = _manager.CreatorFromCookie(req);
    if (auto owner = _manager.CreatorFromUsername(_manager.GetUserForGame(game_id))) {
      if ((*owner)->username() == creator->username()) {
        resp.set_redirect("/?msg=You are the owner of this game.", 303);
      } else {
        (*owner)->AddRequest(creator->username(), game_id);
        resp.set_redirect("/?msg=Successfully requested access.", 303);
      }
    } else {
      resp.set_redirect("/?msg=Owner of game not found!", 303);
    }
  } catch (_http::_t_exception e) {
    resp.set_redirect("/?msg=" + e.second, 303);
  }
}

void Builder::ApiAcceptRequest(const httplib::Request& req, httplib::Response& resp) {
  try {
    auto creator = _manager.CreatorFromCookie(req);
    creator->RemoveRequest(_http::Get(req, "username"), _http::Get(req, "game_id"));
    if (auto creator_rec = _manager.CreatorFromUsername(_http::Get(req, "username"))) {
      (*creator_rec)->AddShared(_http::Get(req, "game_id"));
      resp.set_redirect(_http::Referer(req, "Request accepted."), 303);
      return;
    } 
    resp.set_redirect(_http::Referer(req, "Accepting request failed."), 303);
  } catch (_http::_t_exception e) {
    resp.set_redirect(_http::Referer(req, e.second), 303);
  }
}

void Builder::ApiDenyRequest(const httplib::Request& req, httplib::Response& resp) {
  try {
    auto creator = _manager.CreatorFromCookie(req);
    creator->RemoveRequest(_http::Get(req, "username"), _http::Get(req, "game_id"));
    resp.set_redirect(_http::Referer(req, "Request denied."), 303);
  } catch (_http::_t_exception e) {
    resp.set_redirect(_http::Referer(req, e.second), 303);
  }
}

void Builder::ApiHidePath(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  auto creator = _manager.CreatorFromCookie(req);
  if (creator->OwnsGame(game_id)) {
    const std::string& path = _http::Get(req, "target_path"); 
    std::unique_lock ul(_mtx_games);
    _games.at(game_id)->AddHiddenPath(path);
    resp.set_redirect(_http::Referer(req, "Set \"" + path + "\" as hidden for collaborators."), 303);
  } else {
    resp.set_redirect(_http::Referer(req, "You are not the owner of the game."), 303);
  }
}

void Builder::ApiUnhidePath(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  auto creator = _manager.CreatorFromCookie(req);
  if (creator->OwnsGame(game_id)) {
    const std::string& path = _http::Get(req, "target_path"); 
    std::unique_lock ul(_mtx_games);
    _games.at(game_id)->RemoveHiddenPath(path);
    resp.set_redirect(_http::Referer(req, "Removed \"" + path + "\" from hidden."), 303);
  } else {
    resp.set_redirect(_http::Referer(req, "You are not the owner of the game."), 303);
  }
}

void Builder::ApiReloadGame(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!_manager.CreatorFromCookie(req)->HasAccessToGame(game_id))
    throw _http::_t_exception({400, "No Access to game: \"" + game_id + "\""});
  if (auto res = _cli.Get("/api/game/reload/" + game_id)) {
    if (res->status == httplib::StatusCode::OK_200) {
      resp.set_redirect(_http::Referer(req, "Successfully reloaded game!"), 303);
      std::unique_lock ul(_mtx_games);
      if (_games.contains(game_id)) {
        _games.at(game_id)->set_running(true);
      }
    } else {
      resp.set_redirect(_http::Referer(req, "Reloading game failed with status: " + std::to_string(res->status)), 303);
    }
    return;
  }
  resp.set_redirect(_http::Referer(req, "Reloading game failed: Error with game-server."), 303);
}

void Builder::ApiStopGame(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!_manager.CreatorFromCookie(req)->HasAccessToGame(game_id))
    throw _http::_t_exception({400, "No Access to game: \"" + game_id + "\""});
  if (auto res = _cli.Get("/api/game/stop/" + game_id)) {
    if (res->status == httplib::StatusCode::OK_200) {
      resp.set_redirect(_http::Referer(req, "Successfully stoped game!"), 303);
      std::unique_lock ul(_mtx_games);
      if (_games.contains(game_id)) {
        _games.at(game_id)->set_running(false);
      }
    } else {
      resp.set_redirect(_http::Referer(req, "Stopping game failed with status: " + std::to_string(res->status)), 303);
    }
    return;
  }
  resp.set_redirect(_http::Referer(req, "Stopping game failed: Error with game-server."), 303);
}

void Builder::ApiRunGame(const httplib::Request& req, httplib::Response& resp) {
  util::Logger()->info("/api/tests/run/:game_id");
  std::string game_id = req.path_params.at("game_id");
  auto test_cases = test_parser::LoadTestCases(game_id);
  std::vector<nlohmann::json> result = nlohmann::json::array();
  if (test_cases.empty()) {
    result.push_back({{"desc", "Run Tests"}, {"success", false}, {"error", "No tests listed"}});
  }  else {
    util::Logger()->info("/api/tests/run/" + game_id + ": running test-cases");
    for (const auto& test_case : test_cases) {
      std::unique_lock ul(_mtx_games);
      std::string test_result = test_case.Run(_games.at(game_id));
      util::Logger()->info("/api/tests/run/" + game_id + ": - result: " + test_result);
      if (test_result != "") {
        result.push_back({{"desc", test_case.desc()}, {"success", false}, {"error", test_result}});
      } else {
        result.push_back({{"desc", test_case.desc()}, {"success", true}});
      }
    }
  }
  resp.set_content(nlohmann::json(result).dump(), "application/json");
  resp.status = 200;
}

void Builder::ApiContextIDs(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::shared_lock sl(_mtx_games);
  
  // Get context IDs 
  auto ks = std::views::keys(_games.at(game_id)->contexts());
  std::vector<std::string> ctx_ids{ks.begin(), ks.end()};

  // Return as JSON
  resp.set_content(nlohmann::json(ctx_ids).dump(), "application/json");
  resp.status = 200;
}

void Builder::ApiTextIDs(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::shared_lock sl(_mtx_games);

  // Get text IDs 
  auto ks = std::views::keys(_games.at(game_id)->texts());
  std::vector<std::string> txt_ids{ks.begin(), ks.end()};

  // return as json
  resp.set_content(nlohmann::json(txt_ids).dump(), "application/json");
  resp.status = 200;
}

void Builder::ApiIDs(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::shared_lock sl(_mtx_games);
  
  // Get text and context IDs 
  auto txt_ks = std::views::keys(_games.at(game_id)->texts());
  auto ctx_ks = std::views::keys(_games.at(game_id)->contexts());

  // Join into one vector
  std::vector<std::string> ids{txt_ks.begin(), txt_ks.end()};
  ids.insert(ids.end(), ctx_ks.begin(), ctx_ks.end());

  // Return as json
  resp.set_content(nlohmann::json(ids).dump(), "application/json");
  resp.status = 200;
}

void Builder::ApiTypes(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::shared_lock sl(_mtx_games);
  
  // Get context IDs 
  auto ks = std::views::keys(_games.at(game_id)->contexts());
  std::vector<std::string> ctx_ids{ks.begin(), ks.end()};

  // Generate types from ids 
  auto types = parser::GetTypesFromIDs(ctx_ids);

  // Return as json
  resp.set_content(nlohmann::json(types).dump(), "application/json");
  resp.status = 200;
}

void Builder::ApiContextTypes(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::shared_lock sl(_mtx_games);
  
  // Get context IDs 
  auto ks = std::views::keys(_games.at(game_id)->contexts());
  std::vector<std::string> ctx_ids{ks.begin(), ks.end()};

  // Generate types from ids 
  auto types = parser::GetTypesFromIDs(ctx_ids);
  // Append context-ids to types 
  types.insert(types.end(), ks.begin(), ks.end());

  // Return as json
  resp.set_content(nlohmann::json(types).dump(), "application/json");
  resp.status = 200;
}

void Builder::ApiContextAttributes(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::shared_lock sl(_mtx_games);

  std::map<std::string, std::vector<std::string>> ctx_attribute_mapping;
  for (const auto& it : _games.at(game_id)->contexts()) {
    auto ks = std::views::keys(it.second->attributes());
    ctx_attribute_mapping[it.first] = std::vector<std::string>{ks.begin(), ks.end()};
  }

   // Return as json
  resp.set_content(nlohmann::json(ctx_attribute_mapping).dump(), "application/json");
  resp.status = 200;
}

void Builder::ApiTypesAttributes(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::shared_lock sl(_mtx_games);
  // Get all types first
  auto ks = std::views::keys(_games.at(game_id)->contexts());
  std::vector<std::string> ctx_ids{ks.begin(), ks.end()};
  auto types = parser::GetTypesFromIDs(ctx_ids);

  // Create map: type -> common attributes
  std::map<std::string, std::vector<std::string>> type_attribute_mapping;
  for (const auto& type : types) {
    std::vector<std::string> common_attributes;
    std::vector<std::string> remove;
    bool init = false;
    for (const auto& [ctx_id, ctx] : _games.at(game_id)->contexts()) {
      // Only context of current type
      if (ctx_id.find(type.substr(1)) == 0) {
        // In init phase add all attributes
        if (!init) {
          for (const auto& [key, attr] : ctx->attributes()) {
            common_attributes.push_back(key);
          } 
          init = true;
        } else {
          for (const auto& attr : common_attributes) {
            if (!ctx->attributes().contains(attr)) {
              remove.push_back(attr);
            }
          }
        }
      }
    }
    for (const auto& it : remove) {
      auto el = std::find(common_attributes.begin(), common_attributes.end(), it);
      if (el != common_attributes.end()) {
        common_attributes.erase(el);
      }
    }
    type_attribute_mapping[type] = common_attributes;
  }

  // Return as json
  resp.set_content(nlohmann::json(type_attribute_mapping).dump(), "application/json");
  resp.status = 200;
}

void Builder::ApiMediaAudios(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::shared_lock sl(_mtx_games);
  
  // Join into one vector
  std::vector<std::string> ids;
  for (const auto& media : _games.at(game_id)->media_files()) {
    const std::string extension = media.substr(media.find_last_of('.') + 1);
    if (util::GetContentType(extension).find("audio") == 0) {
      ids.push_back(media);
    }
  }

  // Return as json
  resp.set_content(nlohmann::json(ids).dump(), "application/json");
  resp.status = 200;
}

void Builder::ApiCustomHandlers(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::shared_lock sl(_mtx_games);

  std::vector<std::string> ids;
  for (const auto& ctx : _games.at(game_id)->contexts()) {
    for (const auto& [_, h] : ctx.second->listeners()) {
      if (h->event().starts_with("#")) {
        ids.push_back(h->event());
      }
    }
  }

  // Return as json
  resp.set_content(nlohmann::json(ids).dump(), "application/json");
  resp.status = 200;
}

void Builder::ApiCtxReferences(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("path")) {
    throw std::invalid_argument("Missing query-parameter: (api-ctx-references) path");
  } 
  std::shared_lock sl(_mtx_games);
  nlohmann::json references = _games.at(game_id)->GetCtxReferences(req.get_param_value("path"));
  resp.status = 200;
  resp.set_content(references.dump(), "application/json");
}

void Builder::ApiTxtReferences(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("path")) {
    throw std::invalid_argument("Missing query-parameter: (api-txt-references) path");
  } 
  std::shared_lock sl(_mtx_games);
  nlohmann::json references = _games.at(game_id)->GetTextReferences(req.get_param_value("path"));
  resp.status = 200;
  resp.set_content(references.dump(), "application/json");
}

void Builder::ApiDirReferences(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("path")) {
    throw std::invalid_argument("Missing query-parameter: (api-txt-references) path");
  } 
  std::shared_lock sl(_mtx_games);
  const std::string path = req.get_param_value("path");
  std::vector<std::string> references;
  for (const auto& it : _games.at(game_id)->texts()) {
    if (it.first.find(path) == 0) {
      const auto& refs = _games.at(game_id)->GetTextReferences(it.first, path);
      references.insert(references.end(), refs.begin(), refs.end());
    }
  }
  for (const auto& it : _games.at(game_id)->contexts()) {
    if (it.first.find(path) == 0) {
      const auto& refs = _games.at(game_id)->GetCtxReferences(it.first, path);
      references.insert(references.end(), refs.begin(), refs.end());
    }
  }
  resp.status = 200;
  resp.set_content(nlohmann::json(references).dump(), "application/json");
}

void Builder::ApiMediaReferences(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("path")) {
    throw std::invalid_argument("Missing query-parameter: (api-txt-references) path");
  } 
  std::shared_lock sl(_mtx_games);
  nlohmann::json references = _games.at(game_id)->GetMediaReferences(req.get_param_value("path"));
  resp.status = 200;
  resp.set_content(references.dump(), "application/json");
}

void Builder::ApiCommitMessage(const httplib::Request& req, httplib::Response& resp) {
  if (!req.has_param("commit_sha")) {
    throw std::invalid_argument("Missing query-parameter: (api-commit-mesage) commit_sha");
  } 
  const std::string commit_sha = req.get_param_value("commit_sha");
  std::string game_path;
  {
    std::string game_id = req.path_params.at("game_id");
    std::shared_lock sl(_mtx_games);
    game_path = _games.at(game_id)->path();
  }

  resp.status = 200;
  resp.set_content(git::commit_message(game_path, commit_sha), "application/json");
}
// PAGES 

void Builder::PagesGame(const httplib::Request& req, httplib::Response& resp) {
  const std::string game_id = req.path_params.at("game_id");
  const std::string type = req.has_param("type") ? req.get_param_value("type") : "";
  const std::string path = (req.has_param("path")) ? util::Strip(req.get_param_value("path"), '/') : "";
  const auto& hidden = _games.at(game_id)->builder_settings()._hidded_dirs;
  try {
    if (!_manager.CreatorFromCookie(req)->HasAccessToGame(game_id, hidden, path))
      throw _http::_t_exception({400, "No Access to game: \"" + game_id + "\""});
    if (type == "CTX") {
      LoadTemplate(resp, "ctx-edit.html", CreateParams(req, game_id, type));
    } else if (type == "TXT") {
      LoadTemplate(resp, "txt-edit.html", CreateParams(req, game_id, type));
    } else {
      auto test_cases = test_parser::LoadTestCases(game_id);
      if (test_cases.size() == 0) {
        test_cases.push_back(TestCase());
      }
      LoadTemplate(resp, "game.html", CreateParams(req, game_id, type, test_cases));
    }
  } catch (_http::_t_exception& e) {
    resp.set_redirect("/?msg=" + e.second, 303);
  }
}

void Builder::LoadMedia(const httplib::Request& req, httplib::Response& resp) {
  const std::string path = req.get_param_value("path");
  const std::string game_id = req.path_params.at("game_id");
  std::shared_lock sl(_mtx_games);
  _http::LoadMediaFile(resp, _games.at(game_id)->path() + "/" + txtad::GAME_FILES + path);
}

// GAMES
void Builder::CreateNewGame(const httplib::Request& req, httplib::Response& resp) {
  const std::string game_id = req.form.get_field("game_id");
  const std::string game_path = txtad::GamesPath() + game_id + "/";

  // Check whether game already exists
  if (_games.contains(game_id)) {
    resp.set_redirect(_http::Referer(req, fmt::format("Failed creating new world ({}): Already exists!", game_id)), 303);
    return;
  }

  // Try creating new game
  try {
    std::filesystem::create_directory(game_path);
    std::filesystem::create_directory(game_path + txtad::GAME_FILES);

    txtad::Settings settings;
    util::WriteJsonToDisc(game_path + "settings.json", settings.ToJson());
    builder::Settings builder_settings;
    util::WriteJsonToDisc(game_path + ".builder", builder_settings.ToJson());
    nlohmann::json test_cases = nlohmann::json::array();
    util::WriteJsonToDisc(game_path + "tests.json", test_cases);

    auto creator = _manager.CreatorFromCookie(req);
    creator->AddGame(game_id);
    _games[game_id] = std::make_shared<BuilderGame>(game_path, game_id);
    resp.set_redirect(_http::Referer(req, fmt::format("Successfully created new world: {}.", game_id)), 303);
  } catch (std::exception& e) {
    // Cleanup in case of failure!
    if (std::filesystem::exists(game_path)) {
      std::filesystem::remove_all(game_path);
    }
    resp.set_redirect(_http::Referer(req, fmt::format("Failed creating new world ({}): {}.", game_id, e.what())), 303);
  }
}

void Builder::DeleteGame(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  if (_games.contains(game_id)) {
    if (std::filesystem::exists(_games.at(game_id)->path())) {
      std::filesystem::remove_all(_games.at(game_id)->path());
    }
    _games.erase(game_id);
    resp.set_redirect(fmt::format("/?msg=Successfully removed world: {}.", game_id), 303);
  } else {
    resp.set_redirect(_http::Referer(req, fmt::format("Failed removing world ({}): Not Found.", game_id)), 303);
  }
}

// SAVE ELEMENTS 

void Builder::SaveSettings(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  try {
    nlohmann::json settings_json = {
      {"initial_events", req.form.get_field("initial_events")},
      {"initial_contexts", req.form.get_fields("initial_contexts")}
    };
    std::unique_lock ul(_mtx_games);
    _games.at(game_id)->set_settings(txtad::Settings(settings_json));
    _games.at(game_id)->AddModified("Updated settings");
    if (const auto game_desc = _http::GetField(req, "game_desc")) {
      _games.at(game_id)->UpdateGameDescription(*game_desc);
    }
    resp.set_redirect(_http::Referer(req, "Successfully saved game settings."), 303);
  } catch (std::exception& e) {
    std::string msg = "Failed saving settings: " + std::string(e.what());
    util::Logger()->warn("/:game_id/save/settings: " + msg);
    resp.set_redirect(_http::Referer(req, msg), 303);
  }
}

void Builder::SaveTests(const httplib::Request& req, httplib::Response& resp) { 
  std::string game_id = req.path_params.at("game_id");
  try {
    int tcs = 0; int ts = 0;
    nlohmann::json j = nlohmann::json::parse(req.body); 
    for (const auto& it : j.at("test_cases").get<std::vector<nlohmann::json>>()) {
      TestCase tc(it);
      tcs++; ts += tc.tests().size();
    }
    util::WriteJsonToDisc(txtad::GamesPath() + game_id + "/" + txtad::GAME_TESTS, j["test_cases"]);
    resp.status = 200;
    resp.set_content("Successfully saved " + std::to_string(tcs) + " test cases with " 
        + std::to_string(ts) + " tests.", "text/txt");
  } catch (std::exception& e) {
    resp.set_content("Failed saving tests: " + (std::string)e.what(), "text/txt");
    resp.status = 400;
  }
}

void Builder::SaveCtxMeta(const httplib::Request& req, httplib::Response& resp) {
  const std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  if (!req.has_param("ctx_id")) {
    throw std::invalid_argument("Missing query-parameter: (attribute) ctx_id");
  } 
  const std::string ctx_id = req.get_param_value("ctx_id");
  try {
    const std::string name = req.form.get_field("name");
    const std::string entry_condition = req.form.get_field("entry_condition");
    const int priority = std::stoi(req.form.get_field("priority"));
    const bool permeable = req.form.has_field("permeable");
    const bool shared = req.form.has_field("shared");
    if (auto ctx = util::get_ptr(_games.at(game_id)->contexts(), ctx_id)) {
      ctx->UpdateMeta(name, entry_condition, priority, permeable, shared);
    }
    _games.at(game_id)->AddModified("Updated " + ctx_id);
    resp.set_redirect(_http::Referer(req, "Successfully updated ctx meta for: " + ctx_id), 303);
  } catch (std::exception& e) {
    std::string msg = fmt::format("Failed saving ctx meta for ctx {}: {}", ctx_id, e.what());
    util::Logger()->error(msg);
    resp.set_redirect(_http::Referer(req, msg), 303);
  }
}

void Builder::SaveAttribute(const httplib::Request& req, httplib::Response& resp) {
  const std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  if (!req.has_param("ctx_id")) {
    throw std::invalid_argument("Missing query-parameter: (attribute) ctx_id");
  } if (!req.has_param("id")) {
    throw std::invalid_argument("Missing query-parameter: (attribute) id");
  }
  const std::string ctx_id = req.get_param_value("ctx_id");
  const std::string attribute_id = req.get_param_value("id");
  try {
    const std::string key = req.form.get_field("key");
    const std::string value = req.form.get_field("value");
    if (attribute_id == builder::NEW_ATTRIBUTE) {
      _games.at(game_id)->contexts().at(ctx_id)->AddAttribute(key);
      _games.at(game_id)->AddModified(fmt::format("Added {}.{} = {}", ctx_id, key, value));
    } else if (attribute_id != key) {
      _games.at(game_id)->contexts().at(ctx_id)->RemoveAttribute(attribute_id);
      _games.at(game_id)->AddModified(fmt::format("Removed {}.{}", ctx_id, attribute_id));
      _games.at(game_id)->contexts().at(ctx_id)->AddAttribute(key);
      _games.at(game_id)->AddModified(fmt::format("Added {}.{} = {}", ctx_id, key, value));
    } else {
      _games.at(game_id)->AddModified(fmt::format("Updated {}.{} = {}", ctx_id, key, value));
    }
    _games.at(game_id)->contexts().at(ctx_id)->SetAttribute(key, value);
    resp.set_redirect(_http::Referer(req, "Successfully updated/added attribute"), 303);
  } catch (std::exception& e) {
    std::string msg = fmt::format("Failed creating attribute {} for ctx {}: {}", attribute_id, 
        ctx_id, e.what());
    util::Logger()->error(msg);
    resp.set_redirect(_http::Referer(req, msg), 303);
  }
}

void Builder::SaveListener(const httplib::Request& req, httplib::Response& resp) {
  const std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  if (!req.has_param("ctx_id")) {
    throw std::invalid_argument("Missing query-parameter: (listener) ctx_id");
  } if (!req.has_param("id")) {
    throw std::invalid_argument("Missing query-parameter: (listener) id");
  }
  const std::string ctx_id = req.get_param_value("ctx_id");
  const std::string listener_id = (req.get_param_value("id") == builder::NEW_LISTENER) 
    ? req.form.get_field("id") : req.get_param_value("id");
  try {
    const nlohmann::json j = {
      {"id", req.form.get_field("id")},
      {"re_event", req.form.get_field("event")},
      {"arguments", req.form.get_field("arguments")},
      {"logic", req.form.get_field("logic")},
      {"ctx", req.form.get_field("context")},
      {"use_ctx_regex", std::stoi(req.form.get_field("use_ctx_regex"))},
      {"permeable", req.form.has_field("permeable")},
      {"exec", req.form.has_field("exec")}
    };
    _games.at(game_id)->CreateListenerInPlace(listener_id, j, ctx_id, req.get_param_value("id") == builder::NEW_LISTENER);
    resp.set_redirect(_http::Referer(req, "Successfully updated/added listener"), 303);
  } catch (std::exception& e) {
    std::string msg = fmt::format("Failed creating listener {} for ctx {}: {}", listener_id, 
        ctx_id, e.what());
    util::Logger()->error(msg);
    resp.set_redirect(_http::Referer(req, msg), 303);
  }
}

void Builder::SaveDescriptionElement(const httplib::Request& req, httplib::Response& resp, bool add_new) {
  const std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  if (!req.has_param("ctx_id")) {
    throw std::invalid_argument("Missing query-parameter: (ctx-description) ctx_id");
  } 
  const std::string ctx_id = req.get_param_value("ctx_id");
  try {
    if (auto& ctx = _games.at(game_id)->contexts().at(ctx_id)) {
      ctx->set_description(CreateTextElement(req, ctx->description(), add_new));
      _games.at(game_id)->AddModified(std::string((add_new) ? "Added to" : "Updated") + " description of " + ctx_id);
      resp.set_redirect(_http::Referer(req, "Successfully updated description"), 303);
    } else {
      throw std::invalid_argument("Ctx not found!");
    }
  } catch (std::exception& e) {
    std::string msg = fmt::format("Failed creating text for ctx {}: {}", ctx_id, e.what());
    util::Logger()->error(msg);
    resp.set_redirect(_http::Referer(req, msg), 303);
  }
}

void Builder::SaveTextElement(const httplib::Request& req, httplib::Response& resp, bool add_new) {
  const std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  if (!req.has_param("text_id")) {
    throw std::invalid_argument("Missing query-parameter: (text) text_id");
  } 
  const std::string text_id = req.get_param_value("text_id");
  try {
    const auto& texts = _games.at(game_id)->texts();
    auto updated_txt = CreateTextElement(req, util::get_ptr(texts, text_id), add_new);
    _games.at(game_id)->UpdateText(text_id, updated_txt);
    _games.at(game_id)->AddModified(std::string((add_new) ? "Added to" : "Updated") + " text " + text_id);
    resp.set_redirect(_http::Referer(req, "Successfully " + std::string((add_new) ? "Added to" : "Updated") + " text"), 303);
  } catch (std::exception& e) {
    std::string msg = fmt::format("Failed creating text {}: {}", text_id, e.what());
    util::Logger()->error(msg);
    resp.set_redirect(_http::Referer(req, msg), 303);
  }
}

void Builder::SaveGame(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  std::string game_path = _games.at(game_id)->path();
  try {
    auto changes = _games.at(game_id)->modified();
    _games.at(game_id)->StoreGame();
    _games.erase(game_id);

    const std::string username = _manager.CreatorFromCookie(req)->username();
    if (const auto& branch_name = _http::GetField(req, "branch_name")) {
      git::switch_to_new_branch(game_path, CreateUserBranchname(username, *branch_name));
    }
    git::commit_on_save(game_path, changes, _http::GetField(req, "commit_message"), username);

    // Load game after commiting to be able to load current backup
    _games[game_id] = std::make_shared<BuilderGame>(game_path, game_id);

    resp.status = 200;
  } catch (std::exception& e) {
    resp.set_content("Failed storing game: " + std::string(e.what()), "text/txt");
    resp.status = 400;
  }
}

void Builder::NewElement(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  const auto action = _http::GetField(req, "action");
  const auto id = _http::GetField(req, "id");
  const auto cur_path = (req.has_param("path")) ? req.get_param_value("path") : "";

  if (!action || !id) {
    resp.set_redirect(_http::Referer(req, "Failed creating element. Missing field!"), 303);
    return;
  } 
  if (!util::IsIdType(*id)) {
    resp.set_redirect(_http::Referer(req, "Failed creating element. ID contains invalid characters: " + *id), 303);
    return;
  } 
  // Create id from current path + id
  std::string elem_id = (cur_path.empty()) ? *id : cur_path + "/" + *id;
  if (action == "CTX") {
    _games.at(game_id)->CreateCtx(elem_id);
  } else if (action == "TXT") {
    _games.at(game_id)->CreateTxt(elem_id);
  } else if (action == "DIR") {
    _games.at(game_id)->CreateDir(elem_id);
  } else if (action == "MEDIA") {
    if (!req.form.has_file("formFile")) {
      throw std::runtime_error("Failed creating media element. No file uploaded!");
    }
    // Get file
    const auto& file = req.form.get_file("formFile");
    if (file.filename.empty()) {
      throw std::runtime_error("Failed creating media element. No file selected!");
    }
    // Build media_filename
    std::string media_filename = elem_id + _http::GetFileExtension(file);
    // Save file
    _http::SafeFile(file, _games.at(game_id)->path() + "/" + txtad::GAME_FILES + media_filename);
    // Let game know about media file
    _games.at(game_id)->AddMediaFile(media_filename);
  } else {
    resp.set_redirect(_http::Referer(req, "Failed creating element. Unkwon action: " + *action), 303);
    return;
  }
  resp.set_redirect(_http::Referer(req, "Created new element: " + elem_id + " [" + *action + "]"), 303);
}

// REMOVES ELEMENTS
void Builder::RemoveAttribute(const httplib::Request& req, httplib::Response& resp) {
  const std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  if (!req.has_param("ctx_id")) {
    throw std::invalid_argument("Missing query-parameter: ctx_id");
  } if (!req.has_param("id")) {
    throw std::invalid_argument("/?msg=missing query-parameter or field value: (listener) id");
  }

  try {
    const std::string ctx_id = req.get_param_value("ctx_id");
    const std::string key = req.get_param_value("id");
    _games.at(game_id)->contexts().at(ctx_id)->RemoveAttribute(key);
    _games.at(game_id)->AddModified(fmt::format("Removed {}.{}", ctx_id, key));
    resp.set_redirect(_http::Referer(req, "Successfully removed listener"), 303);
  } catch (std::exception& e) {
    util::Logger()->error("Failed removing listener: " + std::string(e.what()));
    resp.set_content("Failed removing listener: " + std::string(e.what()), "text/txt");
    resp.status = 400;
    return;
  }
}

void Builder::RemoveListener(const httplib::Request& req, httplib::Response& resp) {
  const std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  if (!req.has_param("ctx_id")) {
    throw std::invalid_argument("/?msg=missing query-parameter: ctx_id");
  } if (!req.has_param("id")) {
    throw std::invalid_argument("/?msg=missing query-parameter or field value: (listener) id");
  }

  try {
    const std::string ctx_id = req.get_param_value("ctx_id");
    const std::string id = req.get_param_value("id");
    _games.at(game_id)->RemoveListener(id, ctx_id);
    _games.at(game_id)->AddModified(fmt::format("Removed listener {} for ctx: {}", id, ctx_id));
    resp.set_redirect(_http::Referer(req, "Successfully removed listener"), 303);
  } catch (std::exception& e) {
    util::Logger()->error("Failed removing listener: " + std::string(e.what()));
    resp.set_content("Failed removing listener: " + std::string(e.what()), "text/txt");
    resp.status = 400;
    return;
  }
}

void Builder::RemoveDescriptionElement(const httplib::Request& req, httplib::Response& resp) {
  const std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  if (!req.has_param("ctx_id")) {
    throw std::invalid_argument("Missing query-parameter: (ctx-description) ctx_id");
  } 
  const std::string ctx_id = req.get_param_value("ctx_id");
  try {
    if (auto& ctx = _games.at(game_id)->contexts().at(ctx_id)) {
      ctx->set_description(RemoveTextElement(req, ctx->description()));
      _games.at(game_id)->AddModified("Removed description element of " + ctx_id);
      resp.set_redirect(_http::Referer(req, "Successfully removed description element"), 303);
    } else {
      throw std::invalid_argument("Ctx not found!");
    }
  } catch (std::exception& e) {
    std::string msg = fmt::format("Failed removing description element for ctx {}: {}", ctx_id, e.what());
    util::Logger()->error(msg);
    resp.set_redirect(_http::Referer(req, msg), 303);
  }
}

void Builder::RemoveTextElement(const httplib::Request& req, httplib::Response& resp) {
  const std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  if (!req.has_param("text_id")) {
    throw std::invalid_argument("Missing query-parameter: (text) text_id");
  } 
  const std::string text_id = req.get_param_value("text_id");
  try {
    const auto& texts = _games.at(game_id)->texts();
    _games.at(game_id)->UpdateText(text_id, RemoveTextElement(req, util::get_ptr(texts, text_id)));
    _games.at(game_id)->AddModified("Removed text element " + text_id);
    resp.set_redirect(_http::Referer(req, "Successfully removed text element"), 303);
  } catch (std::exception& e) {
    std::string msg = fmt::format("Failed removing text {}: {}", text_id, e.what());
    util::Logger()->error(msg);
    resp.set_redirect(_http::Referer(req, msg), 303);
  }
}

void Builder::RemoveText(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("path")) {
    throw std::invalid_argument("Missing query-parameter: (remove-txt) path");
  } 
  std::unique_lock ul(_mtx_games);
  const std::string txt_id = req.get_param_value("path");
  _games.at(game_id)->RemoveText(txt_id);
  resp.set_redirect(_http::Referer(req, "Successfully removed text: " + txt_id), 303);
}

void Builder::RemoveContext(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("path")) {
    throw std::invalid_argument("Missing query-parameter: (remove-ctx) path");
  } 
  std::unique_lock ul(_mtx_games);
  const std::string ctx_id = req.get_param_value("path");
  _games.at(game_id)->RemoveContext(ctx_id);
  resp.set_redirect(_http::Referer(req, "Successfully removed context: " + ctx_id), 303);
}

void Builder::RemoveDirectory(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("path")) {
    throw std::invalid_argument("Missing query-parameter: (remove-dir) path");
  } 
  std::unique_lock ul(_mtx_games);
  const std::string path = req.get_param_value("path");
  _games.at(game_id)->RemoveDirectory(path);
  resp.set_redirect(_http::Referer(req, "Successfully removed directory: " + path), 303);
}

void Builder::RemoveMedia(const httplib::Request& req, httplib::Response& resp) {
  const std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("path")) {
    throw std::invalid_argument("Missing query-parameter: (remove-media) path");
  } 
  std::unique_lock ul(_mtx_games);
  const std::string path = req.get_param_value("path");
  _games.at(game_id)->RemoveMedia(path);
  resp.set_redirect(_http::Referer(req, "Successfully removed directory: " + path), 303);
}

void Builder::RestoreGame(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  std::string game_path = _games.at(game_id)->path();
  try {
    _games.at(game_id)->RemovePendingMediaFromDisc(_games.at(game_id)->pending_media_files());
    _games.erase(game_id);
    _games[game_id] = std::make_shared<BuilderGame>(game_path, game_id);
    resp.status = 200;
  } catch (std::exception& e) {
    resp.set_content("Failed restoring old game state: " + std::string(e.what()), "text/txt");
    resp.status = 400;
  }
}

void Builder::RestoreArchive(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("archive")) {
    throw std::invalid_argument("Missing query-parameter: (restore-archive) archive");
  }
  const std::string commit_sha = req.get_param_value("archive");
  std::unique_lock ul(_mtx_games);
  std::string game_path = _games.at(game_id)->path();

  try {
    _games.erase(game_id);
    git::restore_to_commit_always_branch(game_path, commit_sha);
    _games[game_id] = std::make_shared<BuilderGame>(game_path, game_id);
    resp.status = 200;
  } catch (std::exception& e) {
    resp.set_content("Failed restoring archive (" + commit_sha + "): " + std::string(e.what()), "text/txt");
    resp.status = 400;
  }
}

void Builder::DeleteArchive(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("archive")) {
    throw std::invalid_argument("Missing query-parameter: (restore-archive) archive");
  }
  const std::string branch_name = req.get_param_value("archive");
  std::unique_lock sl(_mtx_games);
  git::delete_branch(_games.at(game_id)->path(), branch_name);
  _games.at(game_id)->UpdateBackupInfos();
  resp.set_redirect(_http::Referer(req, "Successfully deleted archive: " + branch_name), 303);
}

void Builder::RenameArchive(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (!req.has_param("archive")) {
    throw std::invalid_argument("Missing query-parameter: (restore-archive) archive");
  }
  const std::string old = req.get_param_value("archive");
  if (auto new_archive = _http::GetField(req, "new_archive")) {
    const std::string username = _manager.CreatorFromCookie(req)->username();
    std::unique_lock ul(_mtx_games);
    git::rename_branch(_games.at(game_id)->path(), old, CreateUserBranchname(username, *new_archive));
    _games.at(game_id)->UpdateBackupInfos();
    resp.set_redirect(_http::Referer(req, "Successfully renamed archive " + old + " -> " + *new_archive), 303);
  } else {
    resp.set_redirect(_http::Referer(req, "Failed renaming archive. Field \"new-archive\" not found!"), 303);
  }
}

// helper 

std::shared_ptr<Text> Builder::CreateTextElement(const httplib::Request& req, std::shared_ptr<Text> text, bool add_new) {
  if (!req.has_param("index")) {
    throw std::invalid_argument("Missing query-parameter: (text) index");
  }
  const int index = std::stoi(req.get_param_value("index"));
  if (index != 0 && !text) {
    throw std::invalid_argument("Trying to add next text to non existing text. Index: " + std::to_string(index));
  }

  // Create new text from json
  const nlohmann::json j = {
    {"txt", req.form.get_field("txt")},
    {"one_time_events", req.form.get_field("one_time_events")},
    {"permanent_events", req.form.get_field("permanent_events")},
    {"logic", req.form.get_field("logic")},
    {"shared", req.form.has_field("shared")}
  };
  util::Logger()->warn("Creating Text-Element from json: {}", j.dump());
  auto new_txt = std::make_shared<Text>(j);
  util::Logger()->warn("Creating Text-Element {} with logic: {}", new_txt->txt(), new_txt->logic());

  // Text did not exist -> return new-text
  if (!text) {
    return new_txt;
  } 
  // New text shall replace first text, add following elements to new text and return
  if (index == 0) {
    if (add_new) {
      new_txt->set_next(text); // inserts new before current text
    } else {
      new_txt->set_next(text->next()); // inserts new inplace of current text
    }
    return new_txt;
  }
  // Otherwise add new text to given index
  if (add_new) {
    text->InsertAt(new_txt, index);
  } else {
    text->ReplaceAt(new_txt, index);
  }
  return text;
}

std::shared_ptr<Text> Builder::RemoveTextElement(const httplib::Request& req, std::shared_ptr<Text> text) {
  if (!req.has_param("index")) {
    throw std::invalid_argument("Missing query-parameter: (text) index");
  }
  const int index = std::stoi(req.get_param_value("index"));
  if (!text) {
    throw std::invalid_argument("Trying to remove text from non existing text. Index: " + std::to_string(index));
  }
  if (auto updated_txt = text->RemoveAt(index)) {
    return updated_txt;
  }
  return std::make_shared<Text>(std::string(""));
}

std::string Builder::CreateUserBranchname(const std::string& username, const std::string& branch_name) {
  std::string iso_utc_now = util::iso_utc_now();
  std::string date = iso_utc_now.substr(0, iso_utc_now.find(" "));
  return username + "/" + date + "--" + branch_name;
}
