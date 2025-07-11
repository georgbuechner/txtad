#include "shared/utils/eventmanager/listener.h"
#include "game/utils/defines.h"
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

// setter 
void LHandler::set_fn(Fn fn) {
  _fn = fn;
}

// methods 
bool LHandler::Test(const std::string& event, const ExpressionParser& parser) const {
  std::smatch base_match;
  return (std::regex_match(event, base_match, static_cast<const std::regex&>(_event)));
}

void LHandler::Execute(std::string event) const {
  if (!_fn) {
    util::Logger()->error("LHandler::Execute. Function from handler: {} not initialized!", _id);
  } else {
    util::Logger()->debug("LHandler::Execute. Executing {}, {}", event, _arguments);
    _fn(event, ReplacedArguments(event, _arguments));
  }
}

std::string LHandler::ReplacedArguments(const std::string& event, const std::string& args) const {
  auto pos = args.find(txtad::EVENT_REPLACEMENT);

  std::smatch base_match;
  if (std::regex_match(event, base_match, static_cast<const std::regex&>(_event))) {
    std::string match = base_match[1].str();
    // If no arguments, ALWAYS return the match
    if (args == "") {
      return match;
    } 
    // If arguments ask for event, replace "#event" with event
    else if (pos != std::string::npos) {
      return util::ReplaceAll(args, txtad::EVENT_REPLACEMENT, match);
    } 
    // Otherwise, return arguments
    else {
      return args;
    }
  }
  return "";
}

// ## l-forwarder
Listener::Fn LForwarder::_overwride_fn = nullptr;

LForwarder::LForwarder(std::string id, std::string re_event, std::string arguments, bool permeable, 
    std::string logic) : LHandler(id, util::ReplaceAll(re_event, txtad::IS_USER_REPLACEMENT, 
        txtad::IS_USER_INP), _overwride_fn, permeable), _logic(logic) { 
    // Remove leading spaces in case of handlers (events starting with #)
  if (_arguments.find(" #") == 0)
    _arguments = arguments.substr(1);
  _arguments = util::ReplaceAll(arguments, "; #", ";#"); 
}

LForwarder::LForwarder(const nlohmann::json& json) : LForwarder(json.at("id"), json.at("re_event"), 
    json.at("arguments"), json.at("permeable"), json.value("logic", "")) { }

// methods 
bool LForwarder::Test(const std::string& event, const ExpressionParser& parser) const {
  if (_logic != "" && parser.Evaluate(LHandler::ReplacedArguments(event, _logic)) != "1")
    return false;
  return LHandler::Test(event, parser);
}

void LForwarder::set_overwite_fn(Fn fn) { 
  _overwride_fn = fn; 
}

// ## l-context-forwarded

LContextForwarder::LContextForwarder(std::string id, std::string re_event, std::weak_ptr<Context> ctx, 
    std::string arguments, bool permeable, UseCtx use_ctx_regex, std::string logic) 
  : LForwarder(id, re_event, util::ReplaceAll(arguments, txtad::CTX_REPLACEMENT, GetCtxId(ctx)), 
      permeable, util::ReplaceAll(logic, txtad::CTX_REPLACEMENT, GetCtxId(ctx))), _ctx(ctx), _use_ctx_regex(use_ctx_regex) {}

LContextForwarder::LContextForwarder(const nlohmann::json& json, std::shared_ptr<Context> ctx) 
  : LContextForwarder(json.at("id"), json.at("re_event"), ctx, json.at("arguments"), json.at("permeable"),
      json.at("use_ctx_regex"), json.value("logic", "")) {
}

// getter 
std::string LContextForwarder::ctx_id () const { return GetCtxId(_ctx); }
std::weak_ptr<Context> LContextForwarder::ctx() const { return _ctx; }

bool LContextForwarder::Test(const std::string& event, const ExpressionParser& parser) const { 
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
        case UseCtx::NAME: 
          return fuzzy::fuzzy(arg, ctx_name) == fuzzy::FuzzyMatch::DIRECT;
        case UseCtx::NAME_FUZZY:
          return fuzzy::fuzzy(arg, ctx_name) == fuzzy::FuzzyMatch::FUZZY;
        case UseCtx::NAME_STARTS_WITH:
          return fuzzy::fuzzy(arg, ctx_name) == fuzzy::FuzzyMatch::STARTS_WITH;
        case UseCtx::NAME_FUZZY_OR_STARTS_WITH:
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
