#include <fmt/format.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include "context.h"
#include "utils/defines.h"
#include "utils/event_manager/event_manager.h"

  // ***** ***** Getters ***** ***** //
std::string Context::name() const {
  return _name;
}

std::string Context::description() const {
  return _description;
}

std::string Context::entry_condition_pattern() const {
  return _entry_condition_pattern;
}
int Context::priority() const {
  return _priority;
}

// ***** ***** Setters ***** ***** //
void Context::set_name(const std::string& name) {
  _name = name;
}

void Context::set_description(const std::string& description) {
  _description = description;
}

void Context::set_entry_condition(const std::string& pattern) {
  _entry_condition_pattern = pattern;
  _entry_condition = std::regex(pattern);
}

  // ***** ***** String representation of the class ***** ***** //
std::string Context::ToString() const {
  return "Name: " + _name + "\n" + "Description: " + _description + "\n" + "Entry Condition (regex): " + _entry_condition_pattern;
}
  
  // ***** ***** Entry check ***** ***** //
bool Context::CheckEntry(const std::string& test) const {
  return std::regex_match(test, _entry_condition);
}

  // ***** ***** Attribute methods ***** ***** //
bool Context::SetAttribute(const std::string& key, const std::string& value) {
  if (_attributes.count(key) == 0) {
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
    util::Logger()->debug(fmt::format("Context::AddAttribute: attribute {} already exists", key));
    return false;
  }
  _attributes[key] = initial_value;
  return true;
}

bool Context::HasAttribute(const std::string& key) const {
  return _attributes.count(key) > 0;
}
  
  // ***** ***** Listener methods calling EventManager ***** ***** //
void Context::AddListener(std::shared_ptr<Listener> listener) {
  if (_event_manager) {
    _event_manager->AddListener(listener);
  } else {
    util::Logger()->debug(fmt::format("Context::AddListener: event_manager does not exist for listener {}", listener));
  }
}

void Context::RemoveListener(const std::string& id) {
  if (_event_manager) {
    _event_manager->RemoveListener(id);
  } else {
    spdlog::get(txtad::LOGGER)->debug("Context::RemoveListener: event_manager does not exist for id {}", id);
}


