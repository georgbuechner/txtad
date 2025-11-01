#include "builder/utils/reflections.h"
#include "builder/creator/creator.h"
#include "builder/creator/manager.h"
#include "builder/utils/defines.h"
#include "builder/utils/jinja_helpers.h"
#include "builder/utils/http_helpers.h"
#include "game/game/game.h"
#include "builder/utils/defines.h"
#include "game/utils/defines.h"
#include "jinja2cpp/filesystem_handler.h"
#include "jinja2cpp/value.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <httplib.h> 
#include <memory>
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

    // Template loader.
    jinja2::TemplateEnv env; 
    jinja2::RealFileSystem fs{builder::TEMPLATE_PATH}; 
    env.AddFilesystemHandler("", fs);

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

    auto create_params = [&](const httplib::Request& req, httplib::Response& resp) -> jinja2::ValuesMap {
      util::Logger()->info(fmt::format("Builder::create_params"));
      jinja2::ValuesMap params;
      params.emplace("games", _jinja::Map(games));
      params.emplace("game_ids", _jinja::MapKeys(games));
      util::Logger()->info(fmt::format("Builder::create_params. checking user"));
      try {
        auto creator = manager.CreatorFromCookie(req, resp);
        params.emplace("creatorname", creator->username());
        params.emplace("worlds", _jinja::SetToVec(creator->worlds()));
        params.emplace("shared", _jinja::SetToVec(creator->shared()));
        params.emplace("pending", _jinja::SetToVec(creator->pending()));
      } catch (_http::_t_exception e) {
        params.emplace("creatorname", "");
      }
      util::Logger()->info(fmt::format("Builder::create_params. checking user done"));
      if (req.get_param_value_count("msg") > 0)
        params.emplace("msg", req.get_param_value("msg"));
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
        auto creator = manager.CreatorFromCookie(req, resp);
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
        auto creator = manager.CreatorFromCookie(req, resp);
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
        auto creator = manager.CreatorFromCookie(req, resp);
        creator->RemoveRequest(_http::Get(req, "username"), _http::Get(req, "game_id"));
        resp.set_redirect(_http::Referer(req) + "?msg=Request denied.", 303);
      } catch (_http::_t_exception e) {
        resp.set_redirect(_http::Referer(req) + "?msg=" + e.second, 303);
      }
    });

    // PAGES
    http_server.Get("/", [&](const httplib::Request& req, httplib::Response& resp) {
      util::Logger()->info(fmt::format("Builder::Root"));
      load_template(resp, "index.html", create_params(req, resp));
    });

    http_server.Get("/login", [&](const httplib::Request& req, httplib::Response& resp) {
      load_template(resp, "login.html", create_params(req, resp));
    });

    http_server.Get("/register", [&](const httplib::Request& req, httplib::Response& resp) {
      load_template(resp, "register.html", create_params(req, resp));
    });

    util::Logger()->info("MAIN: Successfully started http-server on port 4081");
    http_server.listen("0.0.0.0", 4081);
  });

  thread_http.join();
}
