#ifndef SRC_SHARED_OBJECTS_SETTINGS_H_
#define SRC_SHARED_OBJECTS_SETTINGS_H_

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class Settings {
  public: 
    Settings(const nlohmann::json& json) : _initial_events(json.at("initial_events")), 
        _initial_ctx_ids(json.at("initial_contexts").get<std::vector<std::string>>()) {}

    // getter 
    std::string initial_events() const { return _initial_events; }
    const std::vector<std::string>& initial_ctx_ids() const { return _initial_ctx_ids; }

  private: 
    const std::string _initial_events;
    const std::vector<std::string> _initial_ctx_ids;
};

#endif
