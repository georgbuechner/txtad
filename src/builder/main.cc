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
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <exception>
#include <httplib.h> 
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/filesystem_handler.h>

int main() {

  util::SetUpLogger(builder::FILES_PATH, builder::LOGGER, spdlog::level::debug);
  util::LoggerContext scope(builder::LOGGER);

  // Create games
  auto games = parser::InitGames(txtad::GAMES_PATH);

  // Creators
  CreatorManager manager;

  std::thread thread_http([&games, &manager]() {
    httplib::Server http_server;
    http_server.set_mount_point("/static", builder::STATIC_PATH);
    http_server.set_mount_point("/media", builder::MEDIA_PATH);

    // exception handling 
    http_server.set_exception_handler([](const auto& req, auto& resp, std::exception_ptr ep) {
      try {
        std::rethrow_exception(ep);
      } catch (std::exception &e) {
        resp.set_redirect(_http::Referer(req) + "?msg=" + e.what(), 303);
      } catch (...) { // See the following NOTE
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
    env.AddGlobal("default_attribute", jinja2::UserCallable(
        [](auto& params)->jinja2::ValuesMap {
          return jinja2::ValuesMap({{"", ""}});
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

    auto create_params = [&](const httplib::Request& req, std::string game_id = "", std::string type = "") -> jinja2::ValuesMap {
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
        params.emplace("path", path);
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
          resp.set_redirect(_http::Referer(req) + "?msg=Request accepted.", 303);
          return;
        } 
        resp.set_redirect(_http::Referer(req) + "?msg=Accepting request failed.", 303);
      } catch (_http::_t_exception e) {
        resp.set_redirect(_http::Referer(req) + "?msg=" + e.second, 303);
      }
    });

    http_server.Post("/api/creator/requests/deny", [&](const httplib::Request& req, httplib::Response& resp) {
      try {
        auto creator = manager.CreatorFromCookie(req);
        creator->RemoveRequest(_http::Get(req, "username"), _http::Get(req, "game_id"));
        resp.set_redirect(_http::Referer(req) + "?msg=Request denied.", 303);
      } catch (_http::_t_exception e) {
        resp.set_redirect(_http::Referer(req) + "?msg=" + e.second, 303);
      }
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
          load_template(resp, "game.html", create_params(req, game_id, type));
        }
      } catch (_http::_t_exception& e) {
        resp.set_redirect("/?msg=" + e.second, 303);
      }
    });


    util::Logger()->info("MAIN: Successfully started http-server on port 4081");
    http_server.listen("0.0.0.0", 4081);
  });

  thread_http.join();
}
