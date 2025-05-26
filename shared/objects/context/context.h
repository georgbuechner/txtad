#ifndef SRC_OBJECT_CONTEXT_CONTEXT_H
#define SRC_OBJECT_CONTEXT_CONTEXT_H

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
  Context(const std::string& name,
	  const std::string& description,
	  const std::string& entry_condition_pattern,
	  EventManager* event_manager)
    : _name(name),
      _description(description),
      _entry_condition_pattern(entry_condition_pattern),
      _entry_condition(std::regex(entry_condition_pattern)),
      _event_manager(event_manager) {}

  // ***** ***** Getters ***** ***** //
  std::string name() const;
  std::string description() const;
  std::string entry_condition_pattern() const;

  // ***** ***** Setters ***** ***** //
  void set_name(const std::string& name);
  void set_description(const std::string& description);
  void set_entry_condition(const std::string& pattern);

  // ***** ***** String representation of the class ***** ***** //
  std::string ToString() const; 
  
  // ***** ***** Entry check ***** ***** //
  bool CheckEntry(const std::string& test) const;

  // ***** ***** Attribute methods ***** ***** //
  void SetAttribute(const std::string& key, const std::string& value);
  std::optional<std::string> GetAttribute(const std::string& key) const;
  void RemoveAttribute(const std::string& key);
  void AddAttribute(const std::string& key); 
  bool HasAttribute(const std::string& key) const;
  
  // ***** ***** Listener methods calling EventManager ***** ***** //
  void AddListener(std::shared_ptr<Listener> listener);
  void RemoveListener(const std::string& id);

  // ***** ***** Attributes or Data Members ***** ***** //
private:
  std::string _name;
  std::string _description;
  std::string _entry_condition_pattern;
  std::regex _entry_condition;
  std::map<std::string, std::string> _attributes;
  EventManager* _event_manager;

};

#endif
