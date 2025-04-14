#ifndef SRC_UTILS_EVENTMANAGER_EVENTMANAGER_H
#define SRC_UTILS_EVENTMANAGER_EVENTMANAGER_H

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include "listener.h"

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

    void TakeEvent(std::string event) {
      for (const auto& it : _listeners) {
        std::cout << "Checking: " << it.first << std::endl;
        if (it.second->Test(event)) {
          it.second->Execute(event);
          if (!it.second->permeable())
            return;
        }
      }
    }

  private: 
    std::map<std::string, std::shared_ptr<Listener>> _listeners;
};

#endif
