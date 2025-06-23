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

    // getter 
    std::string path() const;
    std::string name() const;
    const std::map<std::string, std::shared_ptr<Context>>& contexts() const;
    const std::map<std::string, std::shared_ptr<Text>>& texts() const;
    const Settings& settings() const;
    
    // setter 
    static void set_msg_fn(MsgFn fn);

    // methods 
    void HandleEvent(const std::string& user_id, const std::string& event);

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
    void h_set_attribute(const std::string& event, const std::string& args);
    void h_add_to_eventqueue(const std::string& event, const std::string& args);

    /**
     * Examples: 
     * #> Hello World.
     * #> You are here: {ctx.rooms/closet->name}. // prints name of ctx "rooms/closet"
     * #> You are here: {ctx.rooms/closet->desc}. // prints description of ctx "rooms/closet"
     * #> You are here: {ctx.*rooms->name}.       // prints name(s) of "rooms"-ctxs
     * #> You are here: {ctx.general.hp}.         // prints attribute "hp" of ctx "general"
     * #> {txt.welcome}                           // Prints text "txt.welcome"
     */
    void h_print(const std::string& event, const std::string& args);
    void h_list_attributes(const std::string& event, const std::string& ctx_id);
    void h_list_all_attributes(const std::string& event, const std::string& ctx_id);

    // tools
    std::string t_substitue_fn(const std::string& str);
};

#endif
