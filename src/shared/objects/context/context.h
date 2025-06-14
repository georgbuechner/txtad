#ifndef SRC_OBJECT_CONTEXT_CONTEXT_H
#define SRC_OBJECT_CONTEXT_CONTEXT_H

#include "shared/utils/eventmanager/eventmanager.h"
#include "shared/utils/eventmanager/listener.h"
#include "shared/utils/utils.h"
#include <string>
#include <regex>
#include <map>
#include <memory>
#include <optional>

  // ***** ***** Forward Declarations ***** ***** //
class EventManager;
class Listener;
class ExpressionParser;

  // ***** ***** Constructor ***** ***** //
class Context {
  
public:
  Context(const std::string& id, int priority, bool permeable=true) : _id(id), _name(""), _description(""), 
      _entry_condition(""), _priority(priority), _permeable(permeable), _event_manager(std::make_unique<EventManager>()) {
    util::Logger()->debug(fmt::format("Context. Context {} created", id)); 
  }

  Context(const std::string& id, const std::string& name, const std::string& description, 
      const std::string& entry_condition_pattern="", int priority=0, bool permeable=true)
    : _id(id), _name(name), _description(description), _entry_condition(entry_condition_pattern), 
      _priority(priority), _permeable(permeable), _event_manager(std::make_unique<EventManager>()) {
    util::Logger()->debug(fmt::format("Context. Context {} created", id)); 
  }

  // ***** ***** Getters ***** ***** //
  std::string id() const;
  std::string name() const;
  std::string description() const;
  std::string entry_condition_pattern() const;
  int priority() const;
  bool permeable() const;

  // ***** ***** Setters ***** ***** //
  void set_name(const std::string& name);
  void set_description(const std::string& description);
  void set_entry_condition(const std::string& pattern);

  // ***** ***** String representation of the class ***** ***** //
  std::string ToString() const; 
  
  // ***** ***** Entry check ***** ***** //
  bool CheckEntry(const std::string& test) const;

  // ***** ***** Attribute methods ***** ***** //
  bool SetAttribute(const std::string& key, const std::string& value);
  std::optional<std::string> GetAttribute(const std::string& key) const;
  bool RemoveAttribute(const std::string& key);
  bool AddAttribute(const std::string& key, std::string initial_value=""); 
  bool HasAttribute(const std::string& key) const;
  
  // ***** ***** Listener methods calling EventManager ***** ***** //
  bool TakeEvent(std::string event, const ExpressionParser& parser);

  void AddListener(std::shared_ptr<Listener> listener);
  void RemoveListener(const std::string& id);

  // ***** ***** Attributes or Data Members ***** ***** //
private:
  const std::string _id;
  std::string _name;
  std::string _description;
  util::Regex _entry_condition;
  std::map<std::string, std::string> _attributes;
  int _priority;
  bool _permeable;

  std::unique_ptr<EventManager> _event_manager;
};

#endif
