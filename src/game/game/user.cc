#include "game/game/user.h"
#include "shared/objects/context/context.h"
#include "shared/utils/utils.h"
#include <memory>

User::User(const std::string& game_id, const std::string& id, txtad::MsgFn cout,
    const std::map<std::string, std::shared_ptr<Context>>& contexts, 
    const std::vector<std::string>& initial_contexts) 
    : _game_id(game_id), _id(id), _cout(cout) { 
  int shared = 0;
  int copied = 0;
  util::Logger()->info("User::User({}). Copying/linking contexts...", _id);
  for (const auto& [key, ctx] : contexts) {
    if (ctx->shared()) {
      _contexts[key] = ctx;
      shared++;
    } else {
      _contexts[key] = std::make_shared<Context>(*ctx);
      copied++;
    }
  }
  util::Logger()->info("User::User({}). Shared {} and copied {} contexts", _id, shared, copied);
  for (const auto& it : initial_contexts) {
    if (_contexts.count(it) > 0)
      LinkContextToStack(_contexts.at(it));
    else 
      util::Logger()->error("User::User({}). Invalid initial context found: {}", _id, it);
  }
}

// getter
const std::string& User::id() const {
  return _id;
}

const std::map<std::string, std::shared_ptr<Context>>& User::contexts() {
  return _contexts;
}

std::string& User::event_queue() {
  return _event_queue;
}

const ContextStack& User::context_stack() const { 
  return _context_stack;
}

// methods 
void User::HandleEvent(const std::string& event, const ExpressionParser& parser) {
  _event_queue = event;
  util::Logger()->info("User::HandleEvent ({}): {}", _id, event);
  while (_event_queue != "") {
    _context_stack.TakeEvents(_event_queue, parser);
  }
}

void User::AddToEventQueue(std::string events) {
  if (_event_queue != "") 
    _event_queue += ";";
  _event_queue += events;
}

void User::LinkContextToStack(std::shared_ptr<Context> ctx) {
  _context_stack.insert(ctx);
}

void User::RemoveContext(const std::string& ctx_id) {
  _context_stack.erase(ctx_id);
}

std::shared_ptr<Context> User::GetContext(const std::string& ctx_id) {
  return _context_stack.get(ctx_id);
}

