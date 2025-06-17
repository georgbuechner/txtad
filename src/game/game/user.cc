#include "game/game/user.h"
#include "shared/objects/context/context.h"
#include "shared/utils/utils.h"
#include <memory>

User::User(const std::string& game_id, const std::string& id, const txtad::MsgFn& cout,
    const std::map<std::string, std::shared_ptr<Context>>& contexts, 
    const std::map<std::string, std::shared_ptr<Text>>& texts, 
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
  shared = 0; copied = 0;
  util::Logger()->info("User::User({}). Copying/linking texts...", _id);
  for (const auto& [key, txt] : texts) {
    if (txt->shared()) {
      _texts[key] = txt;
      shared++;
    } else {
      _texts[key] = std::make_shared<Text>(*txt);
      copied++;
    }
  }
  util::Logger()->info("User::User({}). Shared {} and copied {} texts", _id, shared, copied);

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
  if (ctx_id.front() == '*') {
    for (const auto& ctx : _context_stack.find(ctx_id.substr(1))) 
      _context_stack.erase(ctx->id());
  } else if (_context_stack.exists(ctx_id)) {
    _context_stack.erase(ctx_id);
  } else {
    util::Logger()->warn("User::RemoveContext. Context \"{}\" not found.", ctx_id);
  }
}

std::string User::PrintTxt(std::string txt_id) {
  if (_texts.count(txt_id) > 0)
    return _texts.at(txt_id)->print(_event_queue);
  util::Logger()->warn("User::PrintText. Text {} not found", txt_id);
}

std::string User::PrintCtx(std::string ctx_id, std::string what) {
  util::Logger()->debug("User::PrintCtx. printing \"{}\"...", ctx_id);
  std::string txt;
  auto add_to_txt = [&txt, &what](const std::shared_ptr<Context>& ctx) {
    if (what == "name") 
      txt += ctx->name();
    else if (what == "desc" || what == "description")
      txt += ctx->description();
  };
  if (ctx_id.front() == '*') {
    for (const auto& ctx : _context_stack.find(ctx_id.substr(1)))
      add_to_txt(ctx);
  } else if (_contexts.count(ctx_id) > 0) {
    add_to_txt(_contexts.at(ctx_id));
  } else {
    util::Logger()->warn("User::PrintCtx. Context \"{}\" not found.", ctx_id);
  }
  return txt;
}

std::shared_ptr<Context> User::GetContext(const std::string& ctx_id) {
  return _context_stack.get(ctx_id);
}
