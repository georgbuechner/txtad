#ifndef SRC_BUILDER_USER_CREATOR_H
#define SRC_BUILDER_USER_CREATOR_H 

#include "builder/utils/defines.h"
#include "shared/utils/utils.h"
#include <nlohmann/json.hpp>
#include <string>
#include <set>
#include <vector>

class Creator { 
  public: 
    Creator(std::string username, std::string password) : _username(username), _password(password) {
      Store();
    }
    Creator(nlohmann::json json) : _username(json.at("username")), _password(json.at("password")) {
      for (const auto& game : json.value("games", std::vector<std::string>())) 
        _games.insert(game);
      for (const auto& game : json.value("shared", std::vector<std::string>())) 
        _shared.insert(game);
    }

    // getter 
    std::string username() { return _username; } 
    std::string password() { return _password; } 

    // Methods 
    void Store() {
      nlohmann::json creator_json = { {"username", _username}, {"password", _password} };
      util::WriteJsonToDisc(builder::FILES_PATH + builder::CREATORS_PATH + _username + ".json", creator_json);
    }
    bool HasAccessToGame(const std::string& game_id) {
      return _games.count(game_id) > 0 || _shared.count(game_id);
    }

    bool OwnsGame(const std::string& game_id) {
      return _games.count(game_id) > 0;
    }

    void AddGame(std::string game_id) {
      _games.insert(game_id);
    }

    void AddShared(std::string game_id) {
      _shared.insert(game_id);
    }

    void AddRequest(std::string username, std::string game_id) {
      _pending.insert({username, game_id});
    }

    void AcceptRequest(std::string username, std::string game_id) {
      _pending.erase({username, game_id});
    }

  private:
    const std::string _username;
    std::string _password;

    std::set<std::string> _games;
    std::set<std::string> _shared;
    std::set<std::pair<std::string, std::string>> _pending;
};

#endif
