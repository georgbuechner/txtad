#include <fmt/core.h>
#include <fmt/format.h>
#include <regex>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include "context.h"
#include "shared/utils/eventmanager/eventmanager.h"
#include "shared/utils/eventmanager/listener.h"
#include "shared/utils/utils.h"


  // ***** ***** Getters ***** ***** //
std::string Context::id() const {
  return _id;
}

std::string Context::name() const {
  return _name;
}

std::string Context::description() const {
  return _description.txt();
}

std::string Context::entry_condition_pattern() const {
  return _entry_condition.str();
}
const std::map<std::string, std::string>& Context::attributes() const {
  return _attributes;
}
int Context::priority() const {
  return _priority;
}
bool Context::permeable() const {
  return _permeable;
}
bool Context::shared() const {
  return _shared;
}

// ***** ***** Setters ***** ***** //
void Context::set_name(const std::string& name) {
  _name = name;
}

void Context::set_description(const Text& txt) {
  _description = txt;
}

void Context::set_entry_condition(const std::string& pattern) {
  _entry_condition = util::Regex(pattern);
}

  // ***** ***** String representation of the class ***** ***** //
std::string Context::ToString() const {
  return "Name: " + _name + "\n" + "Description: " + _description.txt() + "\n" + "Entry Condition (regex): " + _entry_condition.str();
}
std::string Context::PrintDescription(std::string& event_queue) {
  
}
  
  // ***** ***** Entry check ***** ***** //
bool Context::CheckEntry(const std::string& test) const {
  return std::regex_match(test, static_cast<std::regex>(_entry_condition));
}

  // ***** ***** Attribute methods ***** ***** //
bool Context::SetAttribute(const std::string& key, const std::string& value) {
  if (_attributes.count(key) > 0) {
    _attributes[key] = value;
    return true;
  }
  return false;
}

std::optional<std::string> Context::GetAttribute(const std::string& key) const {
  auto it = _attributes.find(key);
  if (it != _attributes.end()) {
    return it->second;
  }
  return std::nullopt; 
}
		 
bool Context::RemoveAttribute(const std::string& key) {
  auto it = _attributes.find(key);
  if (it != _attributes.end()) {
    _attributes.erase(key);
    return true;
  }
  return false;
}

bool Context::AddAttribute(const std::string& key, std::string initial_value) {
  if (_attributes.count(key) > 0) {
    util::Logger()->debug("Context::AddAttribute: attribute {} already exists", key);
    return false;
  }
  _attributes[key] = initial_value;
  return true;
}

bool Context::HasAttribute(const std::string& key) const {
  return _attributes.count(key) > 0;
}
  
  // ***** ***** Listener methods calling EventManager ***** ***** //

bool Context::TakeEvent(std::string event, const ExpressionParser& parser) {
  return _event_manager->TakeEvent(event, parser);
}

void Context::AddListener(std::shared_ptr<Listener> listener) {
  if (_event_manager) {
    _event_manager->AddListener(listener);
  } else {
    util::Logger()->error("Context::AddListener: event_manager does not exist for listener {}", listener->id());
  }
}

void Context::RemoveListener(const std::string& id) {
  if (_event_manager) {
    _event_manager->RemoveListener(id);
  } else {
    util::Logger()->error("Context::RemoveListener: event_manager does not exist for id {}", id);
  }
}

std::vector<std::weak_ptr<Context>> Context::LinkedContexts(std::string type) {
  std::vector<std::weak_ptr<Context>> linked_contexts;
  for (const auto& it : _event_manager->listeners()) {
    if (type == "" || it.second->ctx_id().find(type) != std::string::npos)
      linked_contexts.push_back(it.second->ctx());
  }
  return linked_contexts;
}
