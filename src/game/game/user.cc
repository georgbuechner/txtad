#include "game/game/user.h"
#include "game/utils/defines.h"
#include "shared/objects/context/context.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/parser/pattern_parser.h"
#include "shared/utils/utils.h"
#include <memory>
#include <optional>
#include <utility>

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
    _event_queue = util::ReplaceAll(_event_queue, txtad::UID_REPLACEMENT, _id);
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

std::string User::PrintTxt(std::string txt_id, const ExpressionParser& parser) {
  if (_texts.count(txt_id) > 0)
    return util::Join(_texts.at(txt_id)->print(_event_queue, parser), txtad::WEB_CMD_ADD_PROMPT);
  util::Logger()->warn("User::PrintText. Text {} not found", txt_id);
}

std::string User::PrintCtx(std::string ctx_id, std::string what, const ExpressionParser& parser) {
  util::Logger()->debug("User::PrintCtx. printing \"{}\"...", ctx_id);
  std::string txt;
  for (const auto& ctx : GetContext(ctx_id, parser)) {
    User::AddVariableToText(ctx, what, txt, _event_queue, parser);
  }
  return txt;
}

std::string User::PrintCtxAttribute(std::string ctx_id, std::string attribute, const ExpressionParser& parser) {
  util::Logger()->debug("User::PrintCtxAttribute. printing \"{}.{}\".", ctx_id, attribute);
  std::string txt;

  auto add_to_txt = [&txt, &attribute](const std::shared_ptr<Context>& ctx) {
    if (auto attr = ctx->GetAttribute(attribute))
      txt += *attr;
    else 
      util::Logger()->warn("User::PrintCtxAttribute: attribute {} not found in ctx {}", attribute, ctx->id());
  };

  int counter = 0;
  for (const auto& ctx : GetContext(ctx_id, parser)) {
    if (counter++ > 0) {
      txt += ";";
    }
    add_to_txt(ctx);
  }
  return txt;
}

void User::AddVariableToText(const std::shared_ptr<Context>& ctx, const std::string& what, 
    std::string& txt, std::string& event_queue, const ExpressionParser& parser) {
  util::Logger()->debug("User::AddVariableToText: {}, {}", ctx->id(), what);
  if (what == "id") {
    txt += ((txt.length() > 0) ? ", " : "") + ctx->id();
  // Print ctx id
  } else if (what == "name") {
    txt += ((txt.length() > 0) ? ", " : "") + ctx->name();
  // Print ctx description
  } else if (what == "desc" || what == "description") {
    txt += ((txt.length() > 0) ? ", " : "") + ctx->PrintDescription(event_queue, parser);
  // Print ctx attributes (or all attributes)
  } else if (what == "attributes" || what == "all_attributes") {
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
  // Print linked contexts
  } else if (what.front() == '*') {
    if (auto print_ctx = pattern::member_access(what)) {
      for (const auto& it : ctx->LinkedContexts(print_ctx->ctx_id.substr(1))) {
        if (auto linked_ctx = it.lock()) {
          if (print_ctx->member_type == pattern::CtxMemberAccess::VARIABLE) {
            User::AddVariableToText(linked_ctx, print_ctx->key, txt, event_queue, parser);
          }
          else if (print_ctx->member_type == pattern::CtxMemberAccess::ATTRIBUTE) {
            if (auto attr = linked_ctx->GetAttribute(print_ctx->key))
              txt += ((txt != "") ? ", " : "") + *attr;
          }
        }
      }
    }
  }
}

std::vector<std::shared_ptr<Context>> User::GetContext(const std::string& ctx_id, const ExpressionParser& parser) {
  std::vector<std::shared_ptr<Context>> ctxs;

  auto pos = ctx_id.find("[");
  std::string id = ctx_id;
  std::string query = "";
  if (pos != std::string::npos && ctx_id.ends_with("]")) {
    id = ctx_id.substr(0, pos);
    query = ctx_id.substr(pos+1, ctx_id.length()-(pos+1)-1);
  }
  util::Logger()->debug("GetContext id: {}, query: {}", id, query);

  if (id.starts_with("**")) {
    for (const auto& it : _contexts) {
      util::Logger()->debug("User::GetContext: {}, {}", it.first, id);
      if (it.first.find(id.substr(2)) == 0) {
        ctxs.push_back(it.second);
      }
    }
  } else if (id.front() == '*') {
    ctxs = _context_stack.find(id.substr(1));
  } else if (_contexts.count(id) > 0) {
    ctxs = {_contexts.at(id)};
  } else {
    util::Logger()->warn("User::GetContext. Context \"{}\" not found.", id);
  }

  bool chose_random = false;
  if (query == txtad::RAN_ELEM) {
    chose_random = true;
  } else if (query != "") {
    util::Logger()->debug("User::GetContext. Fitering {} ctx with query: {}", ctxs.size(), query);
    std::vector<std::shared_ptr<Context>> filtered_ctxs;

    // remove random qualifier
    auto pos = query.find(", " + txtad::RAN_ELEM);
    if (pos != std::string::npos) {
      query = query.substr(0, pos);
      chose_random = true;
      util::Logger()->debug("User::GetContext. reduced query {}", query);
    }

    // Find position before operand to insert '}'
    pos = query.find(" ");
    if (pos == std::string::npos) {
      util::Logger()->warn("User::GetContext. Query opts must be surrounded by spaces! {}", query);
      return {};
    }
    query[pos] = '}';

    // filter contexts not matching query
    for (const auto& ctx : ctxs) {
      if (parser.Evaluate("{" + ctx->id() + query) == "1") {
        filtered_ctxs.push_back(ctx);
      }
    }

    // set to filtered ctx
    ctxs = std::move(filtered_ctxs);
  }

  // Check if choose-random was selected
  if (chose_random) {
    util::Logger()->debug("User::GetContext. returning random from {} ctxs.", ctxs.size());
    int ran = util::ran(0, ctxs.size());
    return {ctxs.at(ran)};
  }
  util::Logger()->debug("User::GetContext. returning {} ctxs.", ctxs.size());
  return ctxs;
}
