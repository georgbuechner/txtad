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

    // getter 
    const std::map<std::string, std::shared_ptr<Listener>>& listeners() {
      return _listeners;
    }

    // Methods 
    void AddListener(std::shared_ptr<Listener> listener) {
      _listeners[listener->id()] = listener;
    }
    void RemoveListener(const std::string& id) {
      if (_listeners.count(id) > 0)
        _listeners.erase(id);
    }

    bool TakeEvent(std::string event, const ExpressionParser& parser) {
      util::Logger()->debug("EventManager::TakeEvent: \"{}\"", event);
      bool accepted = false;
      for (const auto& it : _listeners) {
        if (it.second->Test(event, parser)) {
          accepted = true;
          it.second->Execute(event);
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
