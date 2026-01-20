#include "builder/server/server.h" 
#include "builder/utils/defines.h"
#include "builder/utils/jinja_helpers.h"
#include "game/utils/defines.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <exception>
#include <stdexcept>
#include <string>

using namespace std::chrono_literals;

Builder::Builder(int port, std::string cli_address, int cli_port) : _cli(cli_address, cli_port), _env(builder::TEMPLATE_PATH) {
  // init games
  _games = parser::InitGames(txtad::GAMES_PATH);

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
      util::Logger()->error(e.what());
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

  _srv.Get("/api/game/reload/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiReloadGame(req, resp); });

  _srv.Get("/api/game/stop/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiStopGame(req, resp); });

  _srv.Get("/api/tests/run/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      ApiRunGame(req, resp); });

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

  // REMOVES ELEMENTS
  _srv.Post("/:game_id/remove/ctx/attribute", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveAttribute(req, resp); });

  _srv.Post("/:game_id/remove/ctx/listener", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveListener(req, resp); });

  _srv.Post("/:game_id/remove/ctx/description", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveDescriptionElement(req, resp); });

  _srv.Post("/:game_id/remove/text", [&](const httplib::Request& req, httplib::Response& resp) {
      RemoveTextElement(req, resp); });

  _srv.Post("/:game_id/restore", [&](const httplib::Request& req, httplib::Response& resp) {
      RestoreGame(req, resp); });

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
  params.emplace("games", _jinja::Map(_games));
  params.emplace("game_ids", _jinja::MapKeys(_games));
  try {
    auto creator = _manager.CreatorFromCookie(req);
    params.emplace("creatorname", creator->username());
    params.emplace("worlds", _jinja::SetToVec(creator->worlds()));
    params.emplace("shared", _jinja::SetToVec(creator->shared()));
    params.emplace("pending", _jinja::SetToVec(creator->pending()));
  } catch (_http::_t_exception e) {
    params.emplace("creatorname", "");
  }
  if (req.get_param_value_count("msg") > 0) {
    params.emplace("msg", req.get_param_value("msg"));
  }

  // Add Game Information
  if (game_id != "" && _games.contains(game_id)) {
    std::string path = (req.has_param("path")) ? util::Strip(req.get_param_value("path"), '/') : "";
    util::Logger()->debug(fmt::format("Builder::create_params. Got path: {}", path));
    params.emplace("game", jinja2::Reflect(PtrView<Game>{_games.at(game_id).get()}));
    params.emplace("all_paths", _jinja::Map(parser::GetPaths(txtad::GAMES_PATH + game_id)));
    params.emplace("selections", _jinja::Map(parser::GetPaths(txtad::GAMES_PATH + game_id, path)));
    if (req.has_param("type") && builder::REVERSE_FILE_TYPE_MAP.contains(type)) {
      auto t_type = builder::REVERSE_FILE_TYPE_MAP.at(type);
      if (t_type == builder::FileType::CTX) {
        params.emplace("ctx", jinja2::Reflect(PtrView<Context>{_games.at(game_id)->contexts().at(path).get()}));
      } else if (t_type == builder::FileType::TXT) {
        params.emplace("texts", jinja2::Reflect(PtrView<Text>{_games.at(game_id)->texts().at(path).get()}));
      }
    }
    params.emplace("test_cases", _jinja::Vec(test_cases));
    params.emplace("path", path);
    params.emplace("back", (path.rfind("/") == std::string::npos) ? "" : path.substr(0, path.rfind("/")));
    params.emplace("modified", _jinja::Vec(_games.at(game_id)->modified()));
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
  try{
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
  auto test_cases = parser::LoadTestCases(game_id);
  std::vector<nlohmann::json> result = nlohmann::json::array();
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
  resp.set_content(nlohmann::json(result).dump(), "application/json");
  resp.status = 200;
}

// PAGES 

void Builder::PagesGame(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::string type = req.has_param("type") ? req.get_param_value("type") : "";
  try {
    if (!_manager.CreatorFromCookie(req)->HasAccessToGame(game_id))
      throw _http::_t_exception({400, "No Access to game: \"" + game_id + "\""});
    if (type == "CTX") {
      LoadTemplate(resp, "ctx-edit.html", CreateParams(req, game_id, type));
    } else if (type == "TXT") {
      LoadTemplate(resp, "txt-edit.html", CreateParams(req, game_id, type));
    } else {
      auto test_cases = parser::LoadTestCases(game_id);
      if (test_cases.size() == 0) {
        test_cases.push_back(TestCase());
      }
      LoadTemplate(resp, "game.html", CreateParams(req, game_id, type, test_cases));
    }
  } catch (_http::_t_exception& e) {
    resp.set_redirect("/?msg=" + e.second, 303);
  }
}

// SAVE ELEMENTS 

void Builder::SaveSettings(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  if (req.form.has_field("initial_contexts") > 0) {
    auto ctxs = req.form.get_fields("initial_contexts");
    for (const auto& ctx : ctxs) 
      std::cout << "- " << ctx << std::endl;
  } 
  try {
    nlohmann::json settings_json = {
      {"initial_events", req.form.get_field("initial_events")},
      {"initial_contexts", req.form.get_fields("initial_contexts")}
    };
    std::unique_lock ul(_mtx_games);
    _games.at(game_id)->set_settings(txtad::Settings(settings_json));
    _games.at(game_id)->AddModified("Updated settings");
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
    util::WriteJsonToDisc(txtad::GAMES_PATH + game_id + "/" + txtad::GAME_TESTS, j["test_cases"]);
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
      {"context", req.form.get_field("context")},
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
    _games.at(game_id)->StoreGame();
    _games.erase(game_id);
    _games[game_id] = std::make_shared<Game>(game_path, game_id);
    resp.status = 200;
  } catch (std::exception& e) {
    resp.set_content("Failed storing game: " + std::string(e.what()), "text/txt");
    resp.status = 400;
  }
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

void Builder::RestoreGame(const httplib::Request& req, httplib::Response& resp) {
  std::string game_id = req.path_params.at("game_id");
  std::unique_lock ul(_mtx_games);
  std::string game_path = _games.at(game_id)->path();
  try {
    _games.erase(game_id);
    _games[game_id] = std::make_shared<Game>(game_path, game_id);
    resp.status = 200;
  } catch (std::exception& e) {
    resp.set_content("Failed restoring old game state: " + std::string(e.what()), "text/txt");
    resp.status = 400;
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
  auto new_txt = std::make_shared<Text>(j);

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
  std::cout << "Removing text @ " << index << std::endl;
  return text->RemoveAt(index);
}
