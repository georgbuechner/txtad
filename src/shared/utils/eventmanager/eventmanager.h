#ifndef SRC_UTILS_EVENTMANAGER_EVENTMANAGER_H
#define SRC_UTILS_EVENTMANAGER_EVENTMANAGER_H

#include <map>
#include <memory>
#include <string>
#include "listener.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/utils.h"

class EventManager {
  public: 
    EventManager() {};

    // Methods 
    void AddListener(std::shared_ptr<Listener> listener) {
      _listeners[listener->id()] = listener;
    }
    void RemoveListener(const std::string& id) {
      if (_listeners.count(id) > 0)
        _listeners.erase(id);
    }

    bool TakeEvent(std::string event, const ExpressionParser& parser) {
      bool accepted = false;
      for (const auto& it : _listeners) {
        if (it.second->Test(event, parser)) {
          accepted = true;
          util::Logger()->debug("Calling execute...");
          it.second->Execute(event);
          util::Logger()->debug("Calling execute done.");
          if (!it.second->permeable())
            return true;
        }
      }
      return accepted;
    }

  private: 
    std::map<std::string, std::shared_ptr<Listener>> _listeners;
};

#endif
