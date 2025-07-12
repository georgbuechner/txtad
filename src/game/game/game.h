#ifndef SRC_GAME_GAME_H
#define SRC_GAME_GAME_H 

#include "game/game/user.h"
#include "shared/objects/context/context.h"
#include "shared/objects/settings/settings.h"
#include "shared/objects/text/text.h"
#include "shared/utils/parser/expression_parser.h"
#include <functional>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>

class Game {
  public: 
    using MsgFn = std::function<void(std::string, std::string)>;

    Game(std::string path, std::string name);
    ~Game();

    // getter 
    std::string path() const;
    std::string name() const;
    const std::map<std::string, std::shared_ptr<Context>>& contexts() const;
    const std::map<std::string, std::shared_ptr<Text>>& texts() const;
    const Settings& settings() const;
    const std::shared_ptr<User>& cur_user();
    
    // setter 
    static void set_msg_fn(MsgFn fn);

    // methods 
    void HandleEvent(const std::string& user_id, const std::string& event);
    std::shared_ptr<User> CreateNewUser(std::string user_id);

  private: 
    static MsgFn _cout;

    mutable std::shared_mutex _mutex;  ///< Mutex for connections_.
    const std::string _path;
    const std::string _name;
    std::map<std::string, std::shared_ptr<User>> _users;
    std::shared_ptr<User> _cur_user;

    ExpressionParser _parser;

    Settings _settings;
    std::shared_ptr<Context> _mechanics_ctx;
    std::map<std::string, std::shared_ptr<Context>> _contexts;
    std::map<std::string, std::shared_ptr<Text>> _texts;

    // handlers 
    void h_add_ctx(const std::string& event, const std::string& ctx_id);
    void h_remove_ctx(const std::string& event, const std::string& ctx_id);
    void h_replace_ctx(const std::string& event, const std::string& args);
    void h_set_ctx_name(const std::string& event, const std::string& args);
    void h_set_attribute(const std::string& event, const std::string& args);

    void h_add_to_eventqueue(const std::string& event, const std::string& args);
    void h_exec(const std::string& event, const std::string& args);

    void h_print(const std::string& event, const std::string& args);
    void h_print_no_prompt(const std::string& event, const std::string& args);
    void h_print_to(const std::string& event, const std::string& args);

    void h_list_attributes(const std::string& event, const std::string& ctx_id);
    void h_list_all_attributes(const std::string& event, const std::string& ctx_id);
    void h_list_linked_contexts(const std::string& event, const std::string& ctx_id);

    void h_reset_game(const std::string& event, const std::string& ctx_id);
    void h_reset_user(const std::string& event, const std::string& ctx_id);

    void h_remove_user(const std::string& event, const std::string& ctx_id);

    // tools
    std::string t_substitue_fn(const std::string& str);

    // helpers 
    std::string GetText(std::string event, std::string args);
};

#endif
