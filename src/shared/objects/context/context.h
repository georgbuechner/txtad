#ifndef SRC_OBJECT_CONTEXT_CONTEXT_H
#define SRC_OBJECT_CONTEXT_CONTEXT_H

#include "shared/utils/utils.h"
#include <string>
#include <regex>
#include <map>
#include <memory>
#include <optional>

  // ***** ***** Forward Declarations ***** ***** //
class EventManager;
class Listener;

  // ***** ***** Constructor ***** ***** //
class Context {
  
public:
  Context(const std::string& id, const std::string& name, const std::string& description, 
      const std::string& entry_condition_pattern, int priority, bool permeable, EventManager* event_manager)
    : _id(id), _name(name), _description(description), _entry_condition_pattern(entry_condition_pattern),
      _entry_condition(std::regex(entry_condition_pattern)), _priority(priority), _permeable(permeable), 
      _event_manager(event_manager) {
    util::Logger()->debug(fmt::format("Context. Context {} created", id)); 
  }
  ~Context() { 
    delete _event_manager; 
    util::Logger()->debug(fmt::format("Context. Context {} deleted.", _id)); 
  }

  // ***** ***** Getters ***** ***** //
  std::string id() const;
  std::string name() const;
  std::string description() const;
  std::string entry_condition_pattern() const;
  int priority() const;
  EventManager* event_manager();

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
  void AddListener(std::shared_ptr<Listener> listener);
  void RemoveListener(const std::string& id);

  // ***** ***** Attributes or Data Members ***** ***** //
private:
  const std::string _id;
  std::string _name;
  std::string _description;
  std::string _entry_condition_pattern;
  std::regex _entry_condition;
  std::map<std::string, std::string> _attributes;
  int _priority;
  bool _permeable;

  EventManager* _event_manager;
};

#endif
