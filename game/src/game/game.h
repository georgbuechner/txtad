#ifndef SRC_GAME_GAME_H
#define SRC_GAME_GAME_H 

#include "game/user.h"
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>

class Game {
  public: 
    using MsgFn = std::function<void(std::string, std::string)>;

    Game(std::string path, std::string name);

    // getter 
    std::string path() const;
    std::string name() const;
    
    // setter 
    static void set_msg_fn(MsgFn fn);

    // methods 
    void HandleEvent(const std::string& user_id, const std::string& event);

  private: 
    mutable std::shared_mutex _mutex;  ///< Mutex for connections_.
    const std::string _path;
    const std::string _name;
    std::map<std::string, std::shared_ptr<User>> _users;
    static MsgFn _cout;
};

#endif
