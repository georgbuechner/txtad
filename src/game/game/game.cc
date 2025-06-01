#include "game/game/game.h"
#include "game/game/user.h"
#include "game/utils/defines.h"
#include "shared/utils/utils.h"
#include <mutex>

Game::MsgFn Game::_cout = nullptr;

Game::Game(std::string path, std::string name) 
    : _path(path), _name(name) {
  util::SetUpLogger(txtad::FILES_PATH, _name, spdlog::level::debug);
}

// getter 
std::string Game::path() const { return _path; } 
std::string Game::name() const { return _name; } 

// setter
void Game::set_msg_fn(Game::MsgFn fn) { _cout = fn; }

// methods 
void Game::HandleEvent(const std::string& user_id, const std::string& event) {
  util::LoggerContext scope(_name);

  util::Logger()->debug("Game::HandleEvent: Handling inp: {}", event);
  std::unique_lock ul(_mutex);
  if (_users.count(user_id) == 0) {
    util::Logger()->debug("Game::HandleEvent: Creating new user: {}", user_id);
    _users[user_id] = std::make_shared<User>(_name, user_id, [&_cout = _cout, user_id](const std::string& msg) {
      _cout(user_id, msg);
    });
  } 
  _users.at(user_id)->HandleEvent(event);
}
