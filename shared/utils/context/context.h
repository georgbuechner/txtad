#include <iostream>
#include <string>
#include <regex>
#include <map>
#include <memory>
#include "eventmanager/eventmanager.h"
#include "eventmanager/listener.h"
#include "utils/parser/expression_parser.h"

class Context {

  // ***** ***** Constructor ***** ***** //
public:
  Context(const std::string& name,
	  const std::string& description,
	  const std::string& entry_condition_pattern,
	  EventManager* event_manager)
    : name(name),
      description(description),
      entry_condition_pattern(entry_condition_pattern),
      entry_condition(std::regex(entry_condition_pattern)),
      event_manager(event_manager) {}

  // ***** ***** Getters ***** ***** //
  std::string GetName() const {
    return name;
  }

  std::string GetDescription() const {
      return description;
  }

  std::string GetEntryCondition() const {
      return entry_condition_pattern;
  }

  // ***** ***** Setters ***** ***** //
  void SetName(const std::string& name) {
      this->name = name;
  }

  void SetDescription(const std::string& description) {
      this->description = description;
  }

 void SetEntryCondition(const std::string& pattern) {
   this->entry_condition_pattern = pattern;
   this->entry_condition = std::regex(pattern);
}

  // ***** ***** String representation of the class ***** ***** //
  std::string ToString() const {
      return "Name: " + name + "\n" + "Description: " + description + "\n" + "Entry Condition (regex): " + entry_condition_pattern;
  }
  
  // ***** ***** Entry check ***** ***** //
  bool CheckEntry(const std::string& test) const {
    return std::regex_match(test, entry_condition);
  }

  // ***** ***** Attribute methods ***** ***** //
  void SetAttribute(const std::string& key, const std::string& value) {
    attributes[key] = value;
  }

  std::string GetAttribute(const std::string& key) const {
      auto it = attributes.find(key);
      if (it != attributes.end()) {
	  return it->second;
      }
      return ""; 
  }
		 
  void RemoveAttribute(const std::string& key) {
    attributes.erase(key);
  }

  void AddAttribute(const std::string& key) {
    if (attributes.find(key) == attributes.end()) {
            attributes[key] = ""; 
        }
    }

   bool HasAttribute(const std::string& key) const {
      return attributes.find(key) != attributes.end();
  }
  
  // ***** ***** Listener methods calling EventManager ***** ***** //

  void AddListener(std::shared_ptr<Listener> listener) {
      if (event_manager) {
	  event_manager->AddListener(listener);
      }
  }

  void RemoveListener(const std::string& id) {
      if (event_manager) {
	  event_manager->RemoveListener(id);
      }
  }

  // ***** ***** Attributes or Data Members ***** ***** //
private:
  std::string name;
  std::string description;
  std::string entry_condition_pattern;
  std::regex entry_condition;
  std::map<std::string, std::string> attributes;
  EventManager* event_manager;

};
