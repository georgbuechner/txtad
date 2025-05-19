#include <iostream>
#include "context.h"

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
void Context::SetAttribute(const std::string& key, const std::string& value) {
  if (_attributes.count(key) == 0) {
    _attributes[key] = value;
  }
}

std::string Context::GetAttribute(const std::string& key) const {
      auto it = _attributes.find(key);
      if (it != _attributes.end()) {
	  return it->second;
      }
      return ""; 
}
		 
void Context::RemoveAttribute(const std::string& key) {
    _attributes.erase(key);
}

bool Context::AddAttribute(const std::string& key, std::string initial_value = "") {
  if (_attributes.count(key) > 0) {
    std::cout << "Attribute " << key << " already exists." << std::endl;
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
      }
}

void Context::RemoveListener(const std::string& id) {
      if (_event_manager) {
	  _event_manager->RemoveListener(id);
      }
}


