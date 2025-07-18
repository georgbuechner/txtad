#include "shared/utils/eventmanager/listener.h"
#include "shared/objects/context/context.h"
#include "shared/utils/defines.h"
#include "shared/utils/fuzzy_search/fuzzy.h"
#include "shared/utils/utils.h"
#include <memory>
#include <string>

// ## l-handler

LHandler::LHandler(std::string id, std::string re_event, Fn fn, bool permeable) : _id(id), _event(re_event),
    _arguments(""), _fn(fn), _permeable(permeable) {}

// getter 
std::string LHandler::id() const { return _id; }
std::string LHandler::event() const { return _event.str(); }
bool LHandler::permeable() const { return _permeable; } 

// methods 
bool LHandler::Test(const std::string& event, const ExpressionParser& parser) {
  std::smatch base_match;
  if (std::regex_match(event, base_match, static_cast<const std::regex&>(_event))) {
    if (base_match.size() >= 2)
      _arguments = base_match[1].str();
    return true;
  }
  return false;
}

void LHandler::Execute(std::string event) const {
  if (!_fn) {
    util::Logger()->error("LHandler::Execute. Function from handler: {} not initialized!", _id);
  } else {
    _fn(event, _arguments);
  }
}

// ## l-forwarder
Listener::Fn LForwarder::_overwride_fn = nullptr;

LForwarder::LForwarder(std::string id, std::string re_event, std::string arguments, bool permeable, std::string logic) : 
    LHandler(id, re_event, _overwride_fn, permeable), _logic(logic) { 
  _arguments = arguments; 
}

// methods 
bool LForwarder::Test(const std::string& event, const ExpressionParser& parser) {
  if (_logic != "" && parser.Evaluate(_logic) != "1")
    return false;
  return LHandler::Test(event, parser);
}

void LForwarder::set_overwite_fn(Fn fn) { 
  _overwride_fn = fn; 
}

// ## l-context-forwarded

LContextForwarder::LContextForwarder(std::string id, std::string re_event, std::shared_ptr<Context> ctx, 
    std::string arguments, bool permeable, UseCtx use_ctx_regex, std::string logic) 
  : LForwarder(id, re_event, arguments, permeable, logic), _logic(logic), _ctx(ctx), _use_ctx_regex(use_ctx_regex) {
  static const std::string ctx_placeholder = "<ctx>";
  auto pos = _arguments.find(ctx_placeholder);
  while (pos != std::string::npos) {
    _arguments.replace(pos, ctx_placeholder.length(), _ctx->id());
    pos = _arguments.find(ctx_placeholder, pos++);
  }
}

bool LContextForwarder::Test(const std::string& event, const ExpressionParser& parser) { 
  // Test logic
  if (_logic != "" && parser.Evaluate(_logic) != "1")
    return false;
  // Test regex
  std::smatch base_match;
  if (std::regex_match(event, base_match, static_cast<const std::regex&>(_event))) {
    // Potentially check context name or regex too
    if (base_match.size() == 2) {
      std::string arg = base_match[1].str();
      switch(_use_ctx_regex) {
        case UseCtx::NO: 
          return true;
        case UseCtx::Name: 
          return fuzzy::fuzzy(arg, _ctx->name()) == fuzzy::FuzzyMatch::DIRECT;
        case UseCtx::Name_FUZZY:
          return fuzzy::fuzzy(arg, _ctx->name()) == fuzzy::FuzzyMatch::FUZZY;
        case UseCtx::Name_STARTS_WITH:
          return fuzzy::fuzzy(arg, _ctx->name()) == fuzzy::FuzzyMatch::STARTS_WITH;
        case UseCtx::Name_FUZZY_OR_STARTS_WITH:
          return fuzzy::fuzzy(arg, _ctx->name()) == fuzzy::FuzzyMatch::FUZZY 
            || fuzzy::fuzzy(arg, _ctx->name()) == fuzzy::FuzzyMatch::STARTS_WITH;
        case UseCtx::REGEX: 
          return _ctx->CheckEntry(arg);
      }
    }
    return true;
  }
  return false;
}
