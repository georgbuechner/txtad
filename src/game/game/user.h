#ifndef SRC_GAME_USER_H
#define SRC_GAME_USER_H

#include "game/utils/defines.h"
#include "shared/objects/context/context.h"
#include "shared/objects/text/text.h"
#include "shared/utils/eventmanager/context_stack.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/parser/pattern_parser.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

class User {
  public: 
    User(const std::string& game_id, const std::string& id, const txtad::MsgFn& cout,
        const std::map<std::string, std::shared_ptr<Context>>& contexts, 
        const std::map<std::string, std::shared_ptr<Text>>& text, 
        const std::vector<std::string>& initial_contexts);

    // getter 
    const std::string& id() const;
    const std::map<std::string, std::shared_ptr<Context>>& contexts();
    const std::map<std::string, std::shared_ptr<Text>>& texts();
    std::string& event_queue();
    const ContextStack& context_stack() const;

    // methods 
    void HandleEvent(const std::string& event, const ExpressionParser& parser);
    /**
     * Accepts *<type> syntax, but expects the result to be a single context.
     */
    std::shared_ptr<Context> GetContext(const std::string& ctx_id);

    // Actions
    void AddToEventQueue(std::string events);
    void LinkContextToStack(std::shared_ptr<Context> ctx);
    void RemoveContext(const std::string& ctx_id);
    std::string PrintTxt(std::string id, const ExpressionParser& parser);
    std::string PrintCtx(std::string id, std::string what, const ExpressionParser& parser);
    std::string PrintCtxAttribute(std::string id, std::string what);

    // helpers 
    static void AddVariableToText(const std::shared_ptr<Context>& ctx, const std::string& what, 
        std::string& txt, std::string& event_queue, const ExpressionParser& parser);

  private: 
    const std::string _game_id;
    const std::string _id;
    const txtad::MsgFn _cout;
    std::map<std::string, std::shared_ptr<Context>> _contexts;
    std::map<std::string, std::shared_ptr<Text>> _texts;

    ContextStack _context_stack;
    std::string _event_queue;
    bool _event_handled;
};

#endif
