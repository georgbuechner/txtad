#ifndef SRC_UTILS_EVENTMANAGER_LISTNER_H
#define SRC_UTILS_EVENTMANAGER_LISTNER_H

#include "shared/utils/parser/expression_parser.h"
#include <functional>
#include <iostream>
#include <regex>
#include <string>

class Listener {
  public: 
    using Fn = std::function<void(std::string, std::string)>;

    /**
     * Create new listener 
     * @param[in] id ( [Cat][num] f.e. M1 = default handlers, P1 -> higher priority, G1 -> lower prio
     * @param[in] event (gets regex-checked)
     * @param[in] arguments (passed when executing or replaced by first regex-match match was found)
     * @param[in] fn (function to execute fn(event:str, (agrument|match[0]):str))
     * @param[in] permeable (stop execution if not permeable)
     * @param[in] logic (additional evaluation)
     */
    Listener(std::string id, std::regex event, std::string arguments, Fn fn, bool permeable, std::string logic="") : 
      _id(id), _event(event), _logic(logic), _arguments(arguments), _fn(fn), _permeable(permeable) {}

    // getter 
    std::string id() const { return _id; }
    bool permeable() const { return _permeable; } 

    // methods 
    bool Test(const std::string& event, const ExpressionParser& parser) {
      std::cout << event << " -> " << ", " << _logic << std::endl;
      if (_logic != "" && parser.Evaluate(_logic) != "1")
        return false;
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
    const std::string _logic;
    std::string _arguments;
    const Fn _fn;
    const bool _permeable;
};

#endif
