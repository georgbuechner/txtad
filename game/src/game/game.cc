#include "game/game.h"
#include "game/user.h"
#include "utils/utils.h"
#include <mutex>
#include <spdlog/spdlog.h>

Game::MsgFn Game::_cout = nullptr;

Game::Game(std::string path, std::string name) 
    : _path(path), _name(name) {
  util::SetUpLogger(_name, spdlog::level::debug);
}

// getter 
std::string Game::path() const { return _path; } 
std::string Game::name() const { return _name; } 

// setter
void Game::set_msg_fn(Game::MsgFn fn) { _cout = fn; }

// methods 
void Game::HandleEvent(const std::string& user_id, const std::string& event) {
  spdlog::get(_name)->debug("Game::HandleEvent: Handling inp: {}", event);
  std::unique_lock ul(_mutex);
  if (_users.count(user_id) == 0) {
    spdlog::get(_name)->debug("Game::HandleEvent: Creating new user: {}", user_id);
    _users[user_id] = std::make_shared<User>(_name, user_id, [&_cout = _cout, user_id](const std::string& msg) {
      _cout(user_id, msg);
    });
  } 
  _users.at(user_id)->HandleEvent(event);
}
