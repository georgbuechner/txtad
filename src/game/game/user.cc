#include "game/game/user.h"
#include "game/utils/defines.h"
#include "shared/objects/context/context.h"
#include "shared/utils/utils.h"
#include <memory>
#include <optional>

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

const std::map<std::string, std::shared_ptr<Text>>& User::texts() {
  return _texts;
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
  if (ctx_id.front() == '*') {
    for (const auto& ctx : _context_stack.find(ctx_id.substr(1)))
      User::AddVariableToText(ctx, what, txt);
  } else if (_contexts.count(ctx_id) > 0) {
    User::AddVariableToText(_contexts.at(ctx_id), what, txt);
  } else {
    util::Logger()->warn("User::PrintCtx. Context \"{}\" not found.", ctx_id);
  }
  return txt;
}

std::string User::PrintCtxAttribute(std::string ctx_id, std::string attribute) {
  util::Logger()->debug("User::PrintCtxAttribute. printing \"{}.{}\".", ctx_id, attribute);
  std::string txt;

  auto add_to_txt = [&txt, &attribute](const std::shared_ptr<Context>& ctx) {
    if (auto attr = ctx->GetAttribute(attribute))
      txt += *attr;
    else 
      util::Logger()->warn("User::PrintCtxAttribute: attribute {} not found in ctx {}", attribute, ctx->id());
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

std::optional<User::CtxPrint> User::GetCtxPrint(std::string inp) {
  static const std::regex pattern(txtad::RE_PRINT_CTX);
  std::smatch match;
  if (std::regex_match(inp, match, pattern)) {
    if (match[2].str() == "->") {
      return CtxPrint({txtad::CtxPrint::VARIABLE, match[1].str(), match[3].str()});
    }
    else if (match[2].str() == ".") {
      return CtxPrint({txtad::CtxPrint::ATTRIBUTE, match[1].str(), match[3].str()});
    }   
  }
  return std::nullopt;
}

void User::AddVariableToText(const std::shared_ptr<Context>& ctx, const std::string& what, 
    std::string& txt) {
  util::Logger()->debug("User::AddVariableToText: {}, {}", ctx->id(), what);
  // Print ctx name
  if (what == "name") 
    txt += ((txt.length() > 0) ? ", " : "") + ctx->name();
  // Print ctx description
  else if (what == "desc" || what == "description")
    txt += ((txt.length() > 0) ? ", " : "") + ctx->description();
  // Print ctx attributes (or all attributes)
  else if (what == "attributes" || what == "all_attributes") {
    std::vector<std::string> hidden;
    for (const auto& [key, value] : ctx->attributes()) {
      std::string str =  key + ": " + value;
      if (key.front() != '_')
        txt += ((txt != "") ? ", " : "") + str;
      else 
        hidden.push_back(str);
    }
    if (what == "all_attributes") {
      for (const auto& str : hidden) {
        txt += ((txt != "") ? ", " : "") + str;
      }
    }
  }
  // Print linked contexts
  else if (what.front() == '*') {
    if (auto print_ctx = User::GetCtxPrint(what)) {
      for (const auto& it : ctx->LinkedContexts(print_ctx->_ctx_id.substr(1))) {
        if (auto linked_ctx = it.lock()) {
          if (print_ctx->_kind == txtad::CtxPrint::VARIABLE)
            User::AddVariableToText(linked_ctx, print_ctx->_what, txt);
          else if (print_ctx->_kind == txtad::CtxPrint::ATTRIBUTE) {
            if (auto attr = linked_ctx->GetAttribute(print_ctx->_what))
              txt += ((txt != "") ? ", " : "") + *attr;
          }
        }
      }
    }
  }
}

std::shared_ptr<Context> User::GetContext(const std::string& ctx_id) {
  if (ctx_id.front() == '*') {
    for (const auto& ctx : _context_stack.find(ctx_id.substr(1))) 
      return _context_stack.get(ctx->id());
  } else if (_contexts.count(ctx_id) > 0) {
    return _contexts.at(ctx_id);
  } else {
    return nullptr;
  }
}
