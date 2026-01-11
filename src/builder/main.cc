#include "builder/creator/manager.h"
#include "builder/utils/defines.h"
#include "builder/utils/jinja_helpers.h"
#include "builder/utils/http_helpers.h"
#include "game/game/game.h"
#include "builder/utils/defines.h"
#include "game/utils/defines.h"
#include "jinja2cpp/filesystem_handler.h"
#include "jinja2cpp/reflected_value.h"
#include "jinja2cpp/value.h"
#include "shared/objects/settings/settings.h"
#include "shared/objects/tests/test_case.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <exception>
#include "httplib.h"
#include <mutex>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/filesystem_handler.h>
#include <websocketpp/error.hpp>

using namespace std::chrono_literals;

int main() {

  util::SetUpLogger(builder::FILES_PATH, builder::LOGGER, spdlog::level::debug);
  util::LoggerContext scope(builder::LOGGER);

  // Create games
  std::shared_mutex mtx_games;
  auto games = parser::InitGames(txtad::GAMES_PATH);

  // Creators
  CreatorManager manager;
  
  // Create clinent "talking" to game-server
  httplib::Client cli("localhost", 4080);
  cli.set_connection_timeout(0, 200000);
  cli.set_read_timeout(2,0);
  cli.set_write_timeout(2,0);

  std::thread thread_game_srv([&games, &cli, &mtx_games]() {
      while(true) {
        std::this_thread::sleep_for(1000ms);
        std::unique_lock ul(mtx_games);
        for (auto& [_, game] : games) {
          game->set_running(false);
        }
        if (auto res = cli.Get("/api/games/running")) {
          if (res->status == httplib::StatusCode::OK_200) {
            try {
              const auto& game_info = nlohmann::json::parse(res->body);
              for (const auto& [id, status] : game_info.get<std::map<std::string, bool>>()) {
                if (games.contains(id)) {
                  games.at(id)->set_running(status);
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
  });

  std::thread thread_http([&games, &manager, &cli, &mtx_games]() {
    httplib::Server http_server;
    http_server.set_mount_point("/static", builder::STATIC_PATH);
    http_server.set_mount_point("/media", builder::MEDIA_PATH);

    // exception handling 
    http_server.set_exception_handler([](const auto& req, auto& resp, std::exception_ptr ep) {
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

    // Template loader.
    jinja2::TemplateEnv env; 
    jinja2::RealFileSystem fs{builder::TEMPLATE_PATH}; 
    env.AddFilesystemHandler("", fs);
    env.AddGlobal("starts_with", jinja2::UserCallable(
        [](auto& params)->jinja2::Value {
          return params["s1"].asString().find(params["s2"].asString()) == 0;
        }, std::vector<jinja2::ArgInfo>({jinja2::ArgInfo{"s1"}, jinja2::ArgInfo{"s2"}})
      ));
    env.AddGlobal("contains", jinja2::UserCallable(
        [](auto& params)->jinja2::Value {
          return params["s"].asString().find(params["sub"].asString()) != std::string::npos;
        }, std::vector<jinja2::ArgInfo>({jinja2::ArgInfo{"s"}, jinja2::ArgInfo{"sub"}})
      ));
    env.AddGlobal("safe", jinja2::UserCallable(
        [](auto& params)->jinja2::Value {
          const std::string str = params["s"].asString();
          return util::ReplaceAll(util::ReplaceAll(str, ">", "&gt;"), "<", "&lt;");
        }, std::vector<jinja2::ArgInfo>({jinja2::ArgInfo{"s"}})
      ));

    env.AddGlobal("is_child", jinja2::UserCallable(
        [](auto& params)->jinja2::Value {
          const std::string path = params["path"].asString();
          const std::string cur = params["cur"].asString();
          if (path.find(cur) == 0) {
            return path.substr(cur.length()).find("/") == std::string::npos;
          }
          return false;
        }, std::vector<jinja2::ArgInfo>({jinja2::ArgInfo{"path"}, jinja2::ArgInfo{"cur"}})
      ));
    env.AddGlobal("default_attributes", jinja2::UserCallable(
        [](auto& params)->jinja2::ValuesMap {
          return jinja2::ValuesMap({{"", ""}});
        }, std::vector<jinja2::ArgInfo>()
      ));
    env.AddGlobal("default_test_cases", jinja2::UserCallable(
        [](auto& params)->jinja2::ValuesList {
          std::vector<TestCase> vec = { TestCase() };
          return _jinja::Vec(vec);
        }, std::vector<jinja2::ArgInfo>()
      ));


    auto load_template = [&](httplib::Response& resp, const std::string& page, 
        const jinja2::ValuesMap& params = jinja2::ValuesMap()) -> void {
      auto tpl = env.LoadTemplate(page);
      if (tpl) { 
        auto rendered = tpl->RenderAsString(params);
        if (rendered) {
          resp.status = 200;
          resp.set_content(*rendered, "text/html");
        } else {
          resp.status = 500; 
          resp.set_content(fmt::format("Template Load Error: {}", rendered.error().ToString()), 
              "text/plain"); 
        }
      } else {
        resp.status = 500; 
        resp.set_content(fmt::format("Template Load Error: {}", tpl.error().ToString()), "text/plain"); 
      }
    };

    auto create_params = [&](const httplib::Request& req, std::string game_id = "", 
        std::string type = "", const std::vector<TestCase>& test_cases = std::vector<TestCase>()
    ) -> jinja2::ValuesMap {
      std::shared_lock sl(mtx_games);
      util::Logger()->info(fmt::format("Builder::create_params"));
      jinja2::ValuesMap params;

      // Add general information
      params.emplace("games", _jinja::Map(games));
      params.emplace("game_ids", _jinja::MapKeys(games));
      try {
        auto creator = manager.CreatorFromCookie(req);
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
      if (game_id != "" && games.contains(game_id)) {
        std::string path = (req.has_param("path")) ? util::Strip(req.get_param_value("path"), '/') : "";
        util::Logger()->info(fmt::format("Builder::create_params. Got path: {}", path));
        params.emplace("game", jinja2::Reflect(PtrView<Game>{games.at(game_id).get()}));
        params.emplace("all_paths", _jinja::Map(parser::GetPaths(txtad::GAMES_PATH + game_id)));
        params.emplace("selections", _jinja::Map(parser::GetPaths(txtad::GAMES_PATH + game_id, path)));
        if (req.has_param("type") && builder::REVERSE_FILE_TYPE_MAP.contains(type)) {
          auto t_type = builder::REVERSE_FILE_TYPE_MAP.at(type);
          if (t_type == builder::FileType::CTX) {
            params.emplace("ctx", jinja2::Reflect(PtrView<Context>{games.at(game_id)->contexts().at(path).get()}));
          } else if (t_type == builder::FileType::TXT) {
            params.emplace("texts", jinja2::Reflect(PtrView<Text>{games.at(game_id)->texts().at(path).get()}));
          }
        }
        params.emplace("test_cases", _jinja::Vec(test_cases));
        params.emplace("path", path);
        params.emplace("back", (path.rfind("/") == std::string::npos) ? "" : path.substr(0, path.rfind("/")));
        params.emplace("modified", games.at(game_id)->modified());
      }
      util::Logger()->info(fmt::format("Builder::create_params. done"));
      return params;
    };

    // API
    http_server.Post("/api/creator/register", [&](const httplib::Request& req, httplib::Response& resp) {
      const std::string msg = manager.Register(req, resp);
      util::Logger()->info(fmt::format("Builder::Register: redirecting to {}", (msg != "") ? "/register?msg=" + msg : "/"));
      resp.set_redirect((msg != "") ? "/register?msg=" + msg : "/", 303);
    });

    http_server.Post("/api/creator/login", [&](const httplib::Request& req, httplib::Response& resp) {
      const std::string msg = manager.Login(req, resp);
      resp.set_redirect((msg != "") ? "/login?msg=" + msg : "/", 303);
    });

    http_server.Get("/api/creator/logout", [&](const httplib::Request& req, httplib::Response& resp) {
      manager.Logout(req, resp);
    });

    http_server.Post("/api/creator/requests/add", [&](const httplib::Request& req, httplib::Response& resp) {
      try{
        const std::string game_id = _http::Get(req, "game_id");
        auto creator = manager.CreatorFromCookie(req);
        if (auto owner = manager.CreatorFromUsername(manager.GetUserForGame(game_id))) {
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
    });

    http_server.Post("/api/creator/requests/accept", [&](const httplib::Request& req, httplib::Response& resp) {
      try {
        auto creator = manager.CreatorFromCookie(req);
        creator->RemoveRequest(_http::Get(req, "username"), _http::Get(req, "game_id"));
        if (auto creator_rec = manager.CreatorFromUsername(_http::Get(req, "username"))) {
          (*creator_rec)->AddShared(_http::Get(req, "game_id"));
          resp.set_redirect(_http::Referer(req, "Request accepted."), 303);
          return;
        } 
        resp.set_redirect(_http::Referer(req, "Accepting request failed."), 303);
      } catch (_http::_t_exception e) {
        resp.set_redirect(_http::Referer(req, e.second), 303);
      }
    });

    http_server.Post("/api/creator/requests/deny", [&](const httplib::Request& req, httplib::Response& resp) {
      try {
        auto creator = manager.CreatorFromCookie(req);
        creator->RemoveRequest(_http::Get(req, "username"), _http::Get(req, "game_id"));
        resp.set_redirect(_http::Referer(req, "Request denied."), 303);
      } catch (_http::_t_exception e) {
        resp.set_redirect(_http::Referer(req, e.second), 303);
      }
    });

    http_server.Get("/api/game/reload/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      std::string game_id = req.path_params.at("game_id");
      if (!manager.CreatorFromCookie(req)->HasAccessToGame(game_id))
        throw _http::_t_exception({400, "No Access to game: \"" + game_id + "\""});
      if (auto res = cli.Get("/api/game/reload/" + game_id)) {
        if (res->status == httplib::StatusCode::OK_200) {
          resp.set_redirect(_http::Referer(req, "Successfully reloaded game!"), 303);
          std::unique_lock ul(mtx_games);
          if (games.contains(game_id)) {
            games.at(game_id)->set_running(true);
          }
        } else {
          resp.set_redirect(_http::Referer(req, "Reloading game failed with status: " + std::to_string(res->status)), 303);
        }
        return;
      }
      resp.set_redirect(_http::Referer(req, "Reloading game failed: Error with game-server."), 303);
    });

    http_server.Get("/api/game/stop/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      std::string game_id = req.path_params.at("game_id");
      if (!manager.CreatorFromCookie(req)->HasAccessToGame(game_id))
        throw _http::_t_exception({400, "No Access to game: \"" + game_id + "\""});
      if (auto res = cli.Get("/api/game/stop/" + game_id)) {
        if (res->status == httplib::StatusCode::OK_200) {
          resp.set_redirect(_http::Referer(req, "Successfully stoped game!"), 303);
          std::unique_lock ul(mtx_games);
          if (games.contains(game_id)) {
            games.at(game_id)->set_running(false);
          }
        } else {
          resp.set_redirect(_http::Referer(req, "Stopping game failed with status: " + std::to_string(res->status)), 303);
        }
        return;
      }
      resp.set_redirect(_http::Referer(req, "Stopping game failed: Error with game-server."), 303);
    });

    http_server.Get("/api/tests/run/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      util::Logger()->info("/api/tests/run/:game_id");
      std::string game_id = req.path_params.at("game_id");
      auto test_cases = parser::LoadTestCases(game_id);
      std::vector<nlohmann::json> result = nlohmann::json::array();
      util::Logger()->info("/api/tests/run/" + game_id + ": running test-cases");
      for (const auto& test_case : test_cases) {
        std::unique_lock ul(mtx_games);
        std::string test_result = test_case.Run(games.at(game_id));
        util::Logger()->info("/api/tests/run/" + game_id + ": - result: " + test_result);
        if (test_result != "") {
          result.push_back({{"desc", test_case.desc()}, {"success", false}, {"error", test_result}});
        } else {
          result.push_back({{"desc", test_case.desc()}, {"success", true}});
        }
      }
      resp.set_content(nlohmann::json(result).dump(), "application/json");
      resp.status = 200;
    });

    // Save Elements 
    http_server.Post("/:game_id/save/settings", [&](const httplib::Request& req, httplib::Response& resp) {
        std::cout << "initial events: " << req.form.get_field("initial_events") << std::endl;
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
          std::unique_lock ul(mtx_games);
          games.at(game_id)->set_settings(Settings(settings_json));
          games.at(game_id)->set_modified(true);
          resp.set_redirect(_http::Referer(req, "Successfully saved game settings."), 303);
        } catch (std::exception& e) {
          std::string msg = "Failed saving settings: " + std::string(e.what());
          util::Logger()->warn("/:game_id/save/settings: " + msg);
          resp.set_redirect(_http::Referer(req, msg), 303);
        }
    });

    http_server.Post("/:game_id/save/tests", [&](const httplib::Request& req, httplib::Response& resp) {
        std::string game_id = req.path_params.at("game_id");
        int tcs = 0;
        int ts = 0;
        nlohmann::json j;
        try {
          j = nlohmann::json::parse(req.body); 
          for (const auto& it : j.at("test_cases").get<std::vector<nlohmann::json>>()) {
            TestCase tc(it);
            tcs++; ts += tc.tests().size();
          }
        } catch (std::exception& e) {
          resp.set_content("Failed saving tests: " + (std::string)e.what(), "text/txt");
          resp.status = 400;
          return;
        }
        util::WriteJsonToDisc(txtad::GAMES_PATH + game_id + "/" + txtad::GAME_TESTS, j["test_cases"]);
        resp.status = 200;
        resp.set_content("Successfully saved " + std::to_string(tcs) + " test cases with " 
            + std::to_string(ts) + " tests.", "text/txt");
    });

    http_server.Post("/:game_id/save/ctx/listener", [&](const httplib::Request& req, httplib::Response& resp) {
      const std::string game_id = req.path_params.at("game_id");
      std::unique_lock ul(mtx_games);
      if (!req.has_param("ctx_id")) {
        resp.set_redirect("/?msg=missing query-parameter: ctx_id", 303);
        return;
      }
      if (!req.has_param("id")) {
        resp.set_redirect("/?msg=missing query-parameter: (listener) id", 303);
        return;
      }
      const std::string ctx_id = req.get_param_value("ctx_id");
      const std::string listener_id = req.get_param_value("id");
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
        games.at(game_id)->CreateListenerInPlace(listener_id, j, ctx_id);
        games.at(game_id)->set_modified(true);
        resp.set_redirect(_http::Referer(req, "Successfully updated/added listener"), 303);
      } catch (std::exception& e) {
        std::string msg = fmt::format("Failed creating listener {} for ctx {}: {}", listener_id, 
            ctx_id, e.what());
        util::Logger()->error(msg);
        resp.set_redirect(_http::Referer(req, msg), 303);
      }
    });

    http_server.Post("/:game_id/save", [&](const httplib::Request& req, httplib::Response& resp) {
      std::string game_id = req.path_params.at("game_id");
      std::unique_lock ul(mtx_games);
      std::string game_path = games.at(game_id)->path();
      try {
        games.at(game_id)->StoreGame();
      } catch (std::exception& e) {
        resp.set_content("Failed storing game: " + std::string(e.what()), "text/txt");
        resp.status = 400;
        return;
      }
      util::Logger()->debug("stored game, erasing... modified was: {}", games.at(game_id)->modified());
      games.erase(game_id);
      util::Logger()->debug("erased game, reloading...");
      games[game_id] = std::make_shared<Game>(game_path, game_id);
      util::Logger()->debug("loaded game. Modified: {}", games.at(game_id)->modified());
      resp.status = 200;
    });

    // PAGES
    http_server.Get("/", [&](const httplib::Request& req, httplib::Response& resp) {
      util::Logger()->info(fmt::format("Builder::Root"));
      load_template(resp, "index.html", create_params(req));
    });

    http_server.Get("/login", [&](const httplib::Request& req, httplib::Response& resp) {
      load_template(resp, "login.html", create_params(req));
    });

    http_server.Get("/register", [&](const httplib::Request& req, httplib::Response& resp) {
      load_template(resp, "register.html", create_params(req));
    });

    http_server.Get("/register", [&](const httplib::Request& req, httplib::Response& resp) {
      load_template(resp, "register.html", create_params(req));
    });

    http_server.Get("/:game_id", [&](const httplib::Request& req, httplib::Response& resp) {
      std::string game_id = req.path_params.at("game_id");
      std::string type = req.has_param("type") ? req.get_param_value("type") : "";
      try {
        if (!manager.CreatorFromCookie(req)->HasAccessToGame(game_id))
          throw _http::_t_exception({400, "No Access to game: \"" + game_id + "\""});
        if (type == "CTX") {
          load_template(resp, "ctx-edit.html", create_params(req, game_id, type));
        } else if (type == "TXT") {
          load_template(resp, "txt-edit.html", create_params(req, game_id, type));
        } else {
          auto test_cases = parser::LoadTestCases(game_id);
          if (test_cases.size() == 0) {
            test_cases.push_back(TestCase());
          }
          load_template(resp, "game.html", create_params(req, game_id, type, test_cases));
        }
      } catch (_http::_t_exception& e) {
        resp.set_redirect("/?msg=" + e.second, 303);
      }
    });

    util::Logger()->info("MAIN: Successfully started http-server on port 4081");
    http_server.listen("0.0.0.0", 4081);
  });

  thread_game_srv.join();
  thread_http.join();
}
