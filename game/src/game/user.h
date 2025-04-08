#ifndef SRC_GAME_USER_H
#define SRC_GAME_USER_H

#include "utils/defines.h"
#include <string>

class User {
  public: 
    User(const std::string& game_id, const std::string& id, txtad::MsgFn cout);

    // methods 
    void HandleEvent(const std::string& event);

  private: 
    const std::string _game_id;
    const std::string _id;
    const txtad::MsgFn _cout;
};

#endif
