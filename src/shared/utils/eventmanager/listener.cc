#include "shared/utils/eventmanager/listener.h"
#include "shared/utils/utils.h"
#include <nlohmann/json.hpp>

// l-handler

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
    if (base_match.size() == 2)
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

// l-forwarder
Listener::Fn LForwarder::_overwride_fn = nullptr;

LForwarder::LForwarder(std::string id, std::string re_event, std::string arguments, bool permeable, std::string logic) : 
    LHandler(id, re_event, _overwride_fn, permeable), _logic(logic) { 
  _arguments = arguments; 
}

LForwarder::LForwarder(const nlohmann::json& json) : LHandler(json.at("id"), json.at("re_event"), _overwride_fn, json.at("permeable")),
    _logic(json.value("logic", "")) {
  _arguments = json.at("arguments");
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
