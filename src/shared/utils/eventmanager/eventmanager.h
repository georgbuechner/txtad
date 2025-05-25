#ifndef SRC_UTILS_EVENTMANAGER_EVENTMANAGER_H
#define SRC_UTILS_EVENTMANAGER_EVENTMANAGER_H

#include <iostream>
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

    /**
     * Takes an event-queue, throws events and modifies event-queue (clears it
     * and eventually adds new events.
     * @param[in, out] event-queue
     * @param[in] parser
     */
    void TakeEvents(std::string& event_queue, const ExpressionParser& parser) {
      std::vector<std::string> events = util::Split(event_queue, ";");
      event_queue = "";
      for (const auto& event : events) {
        TakeEvent(event, parser);
      }
    }

    void TakeEvent(std::string event, const ExpressionParser& parser) {
      for (const auto& it : _listeners) {
        if (it.second->Test(event, parser)) {
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
