#ifndef SRC_BUILDER_USER_CREATOR_H
#define SRC_BUILDER_USER_CREATOR_H 

#include "builder/utils/defines.h"
#include "builder/utils/http_helpers.h"
#include "shared/utils/utils.h"
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
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
      for (const auto& req : json.value("pending", std::vector<std::pair<std::string, std::string>>())) 
        _pending.insert(req);
      util::Logger().get()->info(fmt::format("User {} loaded with {} pending.", _username, _pending.size()));
    }

    // getter 
    std::string username() { return _username; } 
    std::string password() { return _password; } 
    std::set<std::string> worlds() { return _games; } 
    std::set<std::string> shared() { return _shared; } 
    std::set<std::pair<std::string, std::string>> pending() { return _pending; } 

    // setter
    void lock() { _mtx.lock(); }
    void unlock() { _mtx.unlock(); }

    // Methods 
    void Store() {
      nlohmann::json creator_json = { {"username", _username}, {"password", _password} };
      creator_json["games"] = _games; 
      creator_json["shared"] = _shared; 
      creator_json["pending"] = _pending; 
      util::WriteJsonToDisc(builder::CreatorPath() + _username + ".json", creator_json);
    }

    bool HasAccessToGame(const std::string& game_id, const std::set<std::string>& hidden={}, const std::string& path="") {
      if (!path.empty() && util::IsSubPathOf(std::vector<std::string>(hidden.begin(), hidden.end()), path)) {
        return _games.contains(game_id);
      }
      return _games.contains(game_id) || _shared.contains(game_id);
    }

    bool OwnsGame(const std::string& game_id) {
      util::Logger()->info(fmt::format("Comparing {} with {}", game_id, nlohmann::json(_games).dump()));
      return _games.contains(game_id) > 0;
    }

    void AddGame(std::string game_id) {
      _games.insert(game_id);
      Store();
    }

    void AddShared(std::string game_id) {
      _shared.insert(game_id);
      Store();
    }

    void AddRequest(std::string username, std::string game_id) {
      _pending.insert({username, game_id});
      Store();
    }

    void RemoveRequest(std::string username, std::string game_id) {
      if (_pending.contains({username, game_id}) == 0) {
        throw _http::_t_exception({401, "Unkown Game ID: \"" + game_id + "\"."});
      }
      _pending.erase({username, game_id});
      Store();
    }

  private:
    std::mutex _mtx;
    const std::string _username;
    std::string _password;

    std::set<std::string> _games;
    std::set<std::string> _shared;
    std::set<std::pair<std::string, std::string>> _pending;
};

#endif
