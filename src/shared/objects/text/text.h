#ifndef SRC_SHARED_OBJECTS_TEXT_H_
#define SRC_SHARED_OBJECTS_TEXT_H_

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class Text {
  public: 

    Text(std::string txt, std::string one_time_events="", std::string permanent_events="", bool shared=true, std::string logic="", Text* next=nullptr) 
     : _shared(shared), _txt(txt), _one_time_events(one_time_events), _permanent_events(permanent_events), _logic(logic), _next(next) {}

     Text(const nlohmann::json& json) : Text(json.at("txt"), json.value("one_time_events", ""), 
                                            json.value("permanent_events", ""), json.value("shared", true), json.value("logic", ""), nullptr) {}

    // getter 
    bool shared() const { return _shared; }
    std::string txt() const { return _txt; }
    Text* next() const { return _next; }

  
    // methods
  std::vector<std::string> print(std::string& event_queue, const ExpressionParser& parser) {
    std::vector<std::string> txts;
    if (_logic == "" || parser.Evaluate(_logic) == "1") {
      AddEvents(_permanent_events, event_queue);
      AddEvents(_one_time_events, event_queue);
      _one_time_events = "";
      txts.push_back(_txt);
    }
    if (_next) {
      auto more = _next->print(event_queue, parser);
      txts.insert(txts.end(), more.begin(), more.end()); // may need to be improved using std::make_move_iterator for being faster
    }
      return txts;
  }

  private:
    bool _shared;
    std::string _txt;
    std::string _one_time_events;  ///< events only thrown the first time the text is printed
    std::string _permanent_events;   ///< events thrown every time the text is printed
    std::string _logic; ///< logic condition checked before printing
    Text* _next; ///< next text to be printed
  
    static void AddEvents(std::string events, std::string& event_queue) {
      if (events.length() > 0) {
        event_queue += ((event_queue.length() == 0) ? "" : ";") + events;
      }
    }
};

#endif
