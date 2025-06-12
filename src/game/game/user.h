#ifndef SRC_GAME_USER_H
#define SRC_GAME_USER_H

#include "game/utils/defines.h"
#include "shared/objects/context/context.h"
#include "shared/utils/eventmanager/context_stack.h"
#include <memory>
#include <string>
#include <vector>

class User {
  public: 
    User(const std::string& game_id, const std::string& id, txtad::MsgFn cout,
        const std::map<std::string, std::shared_ptr<Context>>& contexts, 
        const std::vector<std::string>& initial_contexts);

    // getter 
    const std::string& id() const;
    const std::map<std::string, std::shared_ptr<Context>>& contexts();
    std::string& event_queue();
    const ContextStack& context_stack() const;

    // methods 
    void HandleEvent(const std::string& event, const ExpressionParser& parser);

    void AddToEventQueue(std::string events);

    void LinkContextToStack(std::shared_ptr<Context> ctx);
    void RemoveContext(const std::string& ctx_id);
    std::shared_ptr<Context> GetContext(const std::string& ctx_id);


  private: 
    const std::string _game_id;
    const std::string _id;
    const txtad::MsgFn _cout;
    std::map<std::string, std::shared_ptr<Context>> _contexts;

    ContextStack _context_stack;
    std::string _event_queue;
};

#endif
