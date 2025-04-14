#ifndef SRC_UTILS_EVENTMANAGER_LISTNER_H
#define SRC_UTILS_EVENTMANAGER_LISTNER_H

#include <functional>
#include <regex>
#include <string>

class Listener {
  public: 
    using Fn = std::function<void(std::string, std::string)>;

    Listener(std::string id, std::regex event, std::string arguments, Fn fn, bool permeable) : 
      _id(id), _event(event), _arguments(arguments), _fn(fn), _permeable(permeable) {}

    // getter 
    std::string id() const { return _id; }
    bool permeable() const { return _permeable; } 

    // methods 
    bool Test(const std::string& event) {
      std::smatch base_match;
      if (std::regex_match(event, base_match, _event)) {
        if (base_match.size() == 2)
          _arguments = base_match[1].str();
        return true;
      }
      return false;
    }

    void Execute(std::string event) const {

      _fn(event, _arguments);
    }

  private: 
    const std::string _id; 
    const std::regex _event;
    std::string _arguments;
    const Fn _fn;
    const bool _permeable;
};

#endif
