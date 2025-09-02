#include "builder/utils/reflections.h"
#include "builder/creator/creator.h"
#include "builder/creator/manager.h"
#include "builder/utils/defines.h"
#include "builder/utils/jinja_helpers.h"
#include "game/game/game.h"
#include "builder/utils/defines.h"
#include "jinja2cpp/filesystem_handler.h"
#include "jinja2cpp/value.h"
#include "shared/utils/utils.h"
#include <algorithm>
#include <filesystem>
#include <httplib.h> 
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/filesystem_handler.h>

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

  util::SetUpLogger(builder::FILES_PATH, builder::LOGGER, spdlog::level::debug);
  util::LoggerContext scope(builder::LOGGER);

  // Create games
  auto games = InitGames();

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
      params.emplace("games", jhelp::Map(games));
      util::Logger()->info(fmt::format("Builder::create_params. checking user"));
      if (auto creator = manager.CreatorFromCookie(req, resp)) {
        params.emplace("creatorname", (*creator)->username());
      } else {
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

    http_server.Post("/api/creator/logout", [&](const httplib::Request& req, httplib::Response& resp) {
      manager.Logout(req, resp);
    });

    http_server.Post("/api/creator/requests/add", [&](const httplib::Request& req, httplib::Response& resp) {
      if (auto creator = manager.CreatorFromCookie(req, resp)) {
        if (auto json = util::ValidateSimpleJson(req.body, {"username", "game"})) {
          (*creator)->AddRequest(json->at("username"), json->at("game"));
        }
      } 
    });

    http_server.Post("/api/creator/requests/accept", [&](const httplib::Request& req, httplib::Response& resp) {
      if (auto creator = manager.CreatorFromCookie(req, resp)) {
        if (auto json = util::ValidateSimpleJson(req.body, {"username", "game"})) {
          (*creator)->AcceptRequest(json->at("username"), json->at("game"));
          if (auto creator_rec = manager.CreatorFromUsername(json->at("username"))) {
            (*creator_rec)->AddShared(json->at("game"));
            resp.status = 200;
          }
        }
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
