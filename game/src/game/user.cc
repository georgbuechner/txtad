#include "game/user.h"
#include <spdlog/spdlog.h>

User::User(const std::string& game_id, const std::string& id, txtad::MsgFn cout) 
  : _game_id(game_id), _id(id), _cout(cout) { }

// methods 
void User::HandleEvent(const std::string& event) {
  spdlog::get(_game_id)->info("User::HandleEvent ({}): {}", _id, event);
  if (event == "new_connection")
    _cout("Username: ");
  else 
    _cout(event);
}

