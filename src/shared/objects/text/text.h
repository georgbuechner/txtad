#ifndef SRC_SHARED_OBJECTS_TEXT_H_
#define SRC_SHARED_OBJECTS_TEXT_H_

#include <nlohmann/json.hpp>
#include <string>
class Text {
  public: 

    Text(std::string txt, std::string one_time_events, std::string permanent_events, bool shared=true) 
      : _shared(shared), _txt(txt), _one_time_events(one_time_events), _permanent_events(permanent_events) {}

    Text(std::string txt) : Text(txt, "", "") {}
    Text(const nlohmann::json& json) : Text(json.at("txt"), json.value("one_time_events", ""), 
        json.value("permanent_events", ""), json.value("shared", true)) {}

    // getter 
    bool shared() const { return _shared; }
    std::string txt() const { return _txt; }

    // methods
    std::string print(std::string& event_queue) { 
      AddEvents(_permanent_events, event_queue);
      AddEvents(_one_time_events, event_queue);
      _one_time_events = "";
      return _txt;
    } 

  private:
    bool _shared;
    std::string _txt;
    std::string _one_time_events;  ///< events only thrown the first time the text is printed
    std::string _permanent_events;   ///< events thrown every time the text is printed

    static void AddEvents(std::string events, std::string& event_queue) {
      if (events.length() > 0) {
        event_queue += ((event_queue.length() == 0) ? "" : ";") + events;
      }
    }
};

#endif
