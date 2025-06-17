#include "game/game/game.h"
#include "game/game/user.h"
#include "game/utils/defines.h"
#include "shared/objects/context/context.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <fmt/core.h>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>

Game::MsgFn Game::_cout = nullptr;

Game::Game(std::string path, std::string name) 
    : _path(path), _name(name), _cur_user(nullptr), _parser(),
    _settings(*util::LoadJsonFromDisc(_path + "/" + txtad::GAME_SETTINGS)) {
  util::SetUpLogger(txtad::FILES_PATH, _name, spdlog::level::debug);
  util::LoggerContext scope(_name);

  // Create baisc handlers
  LForwarder::set_overwite_fn(std::bind(&Game::h_add_to_eventqueue, this, std::placeholders::_1, std::placeholders::_2));

  // Setup mechanics-context
  _mechanics_ctx = std::make_shared<Context>("ctx_mechanic", 0, false);
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H1", "#ctx remove (.*)", 
        std::bind(&Game::h_remove_ctx, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H2", "#ctx add (.*)", 
        std::bind(&Game::h_add_ctx, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H3", "#ctx replace (.*)", 
        std::bind(&Game::h_replace_ctx, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H6", "#> (.*)", 
        std::bind(&Game::h_print, this, std::placeholders::_1, std::placeholders::_2)));

  // Setup parser 
  ExpressionParser::SubstituteFN substitue_fn = [&user=_cur_user](const std::string& str) -> std::string {
    for (const auto& [key, ctx] : user->contexts()) {
      // Check if substitue ist ctx-name 
      if (str == key) 
        return ctx->name();
      // Check if substitue ist ctx-attribute
      if (auto attribute_value = ctx->GetAttribute(str))
        return *attribute_value;
    }
    return "";
  };

  parser::LoadGameFiles(_path, _contexts, _texts);
}

// getter 
std::string Game::path() const { return _path; } 
std::string Game::name() const { return _name; } 
const std::map<std::string, std::shared_ptr<Context>>& Game::contexts() const { return _contexts; }
const std::map<std::string, std::shared_ptr<Text>>& Game::texts() const { return _texts; }
const Settings& Game::settings() const { return _settings; }

// setter
void Game::set_msg_fn(Game::MsgFn fn) { _cout = fn; }

// methods 
void Game::HandleEvent(const std::string& user_id, const std::string& event) {
  util::LoggerContext scope(_name);

  util::Logger()->debug("Game::HandleEvent: Handling inp: {}", event);
  std::unique_lock ul(_mutex);

  // If user did not exist yet, create new user
  if (_users.count(user_id) == 0) {
    util::Logger()->debug("Game::HandleEvent: Creating new user: {}", user_id);
    // Create new user
    auto new_user = std::make_shared<User>(_name, user_id, [&_cout = _cout, user_id](const std::string& msg) {
      _cout(user_id, msg);
    }, _contexts, _texts, _settings.initial_ctx_ids());
    // Link base Context
    util::Logger()->debug("Link base Context.");
    new_user->LinkContextToStack(_mechanics_ctx);
    // Set new user
    util::Logger()->debug("Set new user");
    _users[user_id] = new_user;
    _cur_user = new_user;
    _cur_user->HandleEvent(_settings.initial_events(), _parser);
  } 
  // Otherwise, handle incomming event
  else {
    _cur_user = _users.at(user_id);
    _cur_user->HandleEvent(event, _parser);
  }
}

// handlers

void Game::h_add_ctx(const std::string& event, const std::string& ctx_id) {
  util::Logger()->info("Adding context: {}", ctx_id);
  if (_cur_user->contexts().count(ctx_id) > 0)
    _cur_user->LinkContextToStack(_cur_user->contexts().at(ctx_id));
  else 
    util::Logger()->warn("Handler::link_ctx. Context \"{}\" not found.", ctx_id);
}

void Game::h_remove_ctx(const std::string& event, const std::string& ctx_id) {
  util::Logger()->info("Removing context: {}", ctx_id);
  _cur_user->RemoveContext(ctx_id);
}

void Game::h_replace_ctx(const std::string& event, const std::string& args) {
  static const std::regex pattern("(.*) -> (.*)");
  std::smatch base_match;
  if (std::regex_match(args, base_match, pattern)) {
    if (base_match.size() == 3) {
      h_remove_ctx("", base_match[1].str());
      h_add_ctx("", base_match[2].str());
    }
  }
}

void Game::h_add_to_eventqueue(const std::string& event, const std::string& args) {
  _cur_user->AddToEventQueue(args);
}

void Game::h_print(const std::string& event, const std::string& args) {
  util::Logger()->info("Handler::cout: {} {}", event, args);
  static const std::regex pattern(R"(^(txt)\.(.*)$|^(ctx)\.(.*)->(name|desc|description)$)");
  std::string txt = "";
  for (int i=0; i<args.length();i++) {
    if (args[i] == '{') {
      int closing = util::ClosingBracket(args, i+1, '{', '}');
      if (closing != -1) {
        std::string subsitute = args.substr(i+1, closing-(i+1));
        util::Logger()->info("Handler::cout: found subsitute {}", subsitute);
        if (subsitute == "#event") {
          txt += event;
        } else {
          std::smatch match;
          if (std::regex_match(subsitute, match, pattern)) {
            if (match[1].matched)
              txt += _cur_user->PrintTxt(match[2].str());
            else if (match[3].matched) {
              txt += _cur_user->PrintCtx(match[4].str(), match[5].str());
            }
          } else {
            util::Logger()->info("Handler::cout. {} did not match pattern.", subsitute);
          }
        }
        i = closing;
      } else {
        txt += args[i];
      }
    } else {
      txt += args[i];
    }
  }
  _cout(_cur_user->id(), txt);
}
