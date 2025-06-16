#include "shared/utils/eventmanager/listener.h"
#include "shared/objects/context/context.h"
#include "shared/utils/defines.h"
#include "shared/utils/fuzzy_search/fuzzy.h"
#include "shared/utils/utils.h"
#include <nlohmann/json.hpp>
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
    if (base_match.size() >= 2) {
      util::Logger()->debug("LHandler::Test. Forwared event-match  to arguments {} -> {}", _arguments, base_match[1].str());
      _arguments = base_match[1].str();
    }
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

LForwarder::LForwarder(std::string id, std::string re_event, std::string arguments, bool permeable, 
  std::string logic) : 
    LHandler(id, re_event, _overwride_fn, permeable), _logic(logic) { 
  _arguments = arguments; 
}

LForwarder::LForwarder(const nlohmann::json& json) : LForwarder(json.at("id"), json.at("re_event"), 
    json.at("arguments"), json.at("permeable"), json.value("logic", "")) { }

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

LContextForwarder::LContextForwarder(std::string id, std::string re_event, std::weak_ptr<Context> ctx, 
    std::string arguments, bool permeable, UseCtx use_ctx_regex, std::string logic) 
  : LForwarder(id, re_event, arguments, permeable, logic), _ctx(ctx), _use_ctx_regex(use_ctx_regex) {
  static const std::string ctx_placeholder = "<ctx>";
  auto pos = _arguments.find(ctx_placeholder);
  const std::string ctx_id = GetCtxId(_ctx);
  while (pos != std::string::npos) {
    _arguments.replace(pos, ctx_placeholder.length(), ctx_id);
    pos = _arguments.find(ctx_placeholder, pos++);
  }
}

LContextForwarder::LContextForwarder(const nlohmann::json& json, std::shared_ptr<Context> ctx) 
  : LContextForwarder(json.at("id"), json.at("re_event"), ctx, json.at("arguments"), json.at("permeable"),
      json.at("use_ctx_regex"), json.value("logic", "")) {}

bool LContextForwarder::Test(const std::string& event, const ExpressionParser& parser) { 
  // Test logic
  if (_logic != "" && parser.Evaluate(_logic) != "1")
    return false;
  // Test regex
  std::smatch base_match;
  if (std::regex_match(event, base_match, static_cast<const std::regex&>(_event))) {
    // Potentially check context name or regex too
    if (base_match.size() == 2) {
      const std::string ctx_name = GetCtxName(_ctx);
      std::string arg = base_match[1].str();
      switch(_use_ctx_regex) {
        case UseCtx::NO: 
          return true;
        case UseCtx::Name: 
          return fuzzy::fuzzy(arg, ctx_name) == fuzzy::FuzzyMatch::DIRECT;
        case UseCtx::Name_FUZZY:
          return fuzzy::fuzzy(arg, ctx_name) == fuzzy::FuzzyMatch::FUZZY;
        case UseCtx::Name_STARTS_WITH:
          return fuzzy::fuzzy(arg, ctx_name) == fuzzy::FuzzyMatch::STARTS_WITH;
        case UseCtx::Name_FUZZY_OR_STARTS_WITH:
          return fuzzy::fuzzy(arg, ctx_name) == fuzzy::FuzzyMatch::FUZZY 
            || fuzzy::fuzzy(arg, ctx_name) == fuzzy::FuzzyMatch::STARTS_WITH;
        case UseCtx::REGEX: 
          if (auto ctx = _ctx.lock()) {
            return ctx->CheckEntry(arg);
          } else {
            util::Logger()->error("Context for ContextForwarder {} not availible!", _id);
            return false;
          }
      }
    }
    return true;
  }
  return false;
}    

std::string LContextForwarder::GetCtxId(std::weak_ptr<Context> _ctx) {
  if (auto ctx = _ctx.lock()) {
    return ctx->id();
  } 
  util::Logger()->error("GetCtxId: Context for ContextForwarder not availible!");
  return "";
}

std::string LContextForwarder::GetCtxName(std::weak_ptr<Context> _ctx) {
  if (auto ctx = _ctx.lock()) {
    return ctx->name();
  } 
  util::Logger()->error("GetCtxName: Context for ContextForwarder not availible!");
  return "";
}
