#include "game/game/game.h"
#include "game/game/user.h"
#include "game/utils/defines.h"
#include "shared/objects/context/context.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <fmt/core.h>
#include <memory>
#include <mutex>

Game::MsgFn Game::_cout = nullptr;

Game::Game(std::string path, std::string name) 
    : _path(path), _name(name), _cur_user(nullptr), _parser({}),
    _settings(*util::LoadJsonFromDisc(_path + "/" + txtad::GAME_SETTINGS)) {
  util::SetUpLogger(txtad::FILES_PATH, _name, spdlog::level::debug);
  util::LoggerContext scope(_name);

  parser::LoadGameFiles(_path, _contexts, _texts);

  // Create baisc handlers
  Listener::Fn add_ctx = [&user=_cur_user](std::string event, std::string ctx_id) {
    util::Logger()->info(fmt::format("Adding context: {}", ctx_id));
    if (user->contexts().count(ctx_id) > 0)
      user->LinkContextToStack(user->contexts().at(ctx_id));
    else 
      util::Logger()->warn(fmt::format("Handler::link_ctx. Context \"{}\" not found.", ctx_id));
  };
  Listener::Fn remove_ctx = [&user=_cur_user](std::string event, std::string ctx_id) {
    util::Logger()->info(fmt::format("Adding context: {}", ctx_id));
    if (user->context_stack().exists(ctx_id))
      user->RemoveContext(ctx_id);
    else 
      util::Logger()->warn(fmt::format("Handler::remove_ctx. Context \"{}\" not found.", ctx_id));
  };
  Listener::Fn replace_ctx = [&user=_cur_user](std::string event, std::string args) {
    static const std::regex regex_expression("(.*) -> (.*)");
    std::smatch base_match;
    if (std::regex_match(args, base_match, regex_expression)) {
      if (base_match.size() == 3) {
        std::string ctx_from = base_match[1].str();
        std::string ctx_to = base_match[2].str();
        if (user->context_stack().exists(ctx_from)) {
          user->RemoveContext(ctx_from);
          if (user->contexts().count(ctx_to) > 0)
            user->LinkContextToStack(user->contexts().at(ctx_to));
          else 
            util::Logger()->warn(fmt::format("Handler::replace_ctx. (To-)Context \"{}\" not found.", ctx_to));
        } else {
          util::Logger()->warn(fmt::format("Handler::replace_ctx. (From-)Context \"{}\" not found.", ctx_from));
        }
      }
    }
  };

  Listener::Fn add_to_eventqueue = [&user=_cur_user](std::string event, std::string args) {
    user->AddToEventQueue(args);
  };

  Listener::Fn cout = [&print = _cout, &user=_cur_user](std::string event, std::string args) {
    print(user->id(), args);
  };

  Listener::Fn cout_ctx = [&print = _cout, &user=_cur_user](std::string event, std::string ctx_id) {
    if (auto ctx = user->GetContext(ctx_id))
      print(user->id(), ctx->description());
    else 
      util::Logger()->warn(fmt::format("Handler::cout_ctx. Context \"{}\" not found.", ctx_id));
  };

  Listener::Fn cout_txt = [&print = _cout, &user=_cur_user, &texts=_texts](std::string event, std::string txt_id) {
    if (texts.count(txt_id) > 0)
      print(user->id(), texts.at(txt_id)->print(user->event_queue()));
    else
      util::Logger()->warn(fmt::format("Handler::cout_txt. Text \"{}\" not found.", txt_id));
  };

  _mechanics_ctx = std::make_shared<Context>("ctx_mechanic", 10);
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H1", "#ctx remove (.*)", remove_ctx));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H2", "#ctx add (.*)", add_ctx));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H3", "#ctx replace (.*)", replace_ctx));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H4", "#>ctx (.*)", cout_ctx));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H5", "#>txt (.*)", cout_txt));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H6", "#> (.*)", cout));
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
    }, _contexts, _settings.initial_ctx_ids());
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
