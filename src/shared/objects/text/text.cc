#include "shared/objects/text/text.h"
#include "shared/utils/utils.h"
#include "shared/utils/parser/game_file_parser.h"
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

Text::Text(std::string txt, std::string one_time_events, std::string permanent_events, bool shared, 
    std::string logic, Text* next) 
 : _shared(shared), _txt(txt), _one_time_events(one_time_events), _permanent_events(permanent_events), 
  _logic(logic), _next(next) {}

Text::Text(std::string txt) : Text(txt, "", "") {}

Text::Text(nlohmann::json json, std::string ctx_id) {
  _next = nullptr;
  // If list-style, get first element of list and create next from remaining
  // list.
  if (json.is_array()) {
    std::vector<nlohmann::json> texts = json;
    if (texts.size() > 0) {
      json = texts.front();
      // If there is still a next element, construct from remaining list
      if (texts.size() > 1) {
        _next = std::make_shared<Text>(std::vector<nlohmann::json>(texts.begin()+1, texts.end()));
      }
    } else {
      util::Logger().get()->warn(fmt::format("Text::Text. Empty txt-list: ", json.dump()));
    }
  } 
  _shared = json.value("shared", true);
  _txt = json.at("txt");
  _one_time_events = json.value("one_time_events", "");
  _permanent_events = json.value("permanent_events", "");
  if (!ctx_id.empty()) {
    _one_time_events = parser::DoThisReplacement(_one_time_events, ctx_id);
    _permanent_events = parser::DoThisReplacement(_permanent_events, ctx_id);
  }
  _logic = json.value("logic", "");
}

Text::~Text() { 
  // std::cout << "Text::~Text: Deleting text: " << _txt << std::endl; 
}

// getter 
bool Text::shared() const { return _shared; }
std::string Text::txt() const { return _txt; }
std::string Text::one_time_events() const { return _one_time_events; }
std::string Text::permanent_events() const { return _permanent_events; }
std::string Text::logic() const { return _logic; }
std::shared_ptr<Text> Text::next() const { return _next; }

// setter 
void Text::set_next(std::shared_ptr<Text> txt) { _next = txt; }

// methods
std::vector<std::string> Text::print(std::string& event_queue, const ExpressionParser& parser) {
  std::vector<std::string> txts;
  if (_logic == "" || parser.Evaluate(_logic) == "1") {
    AddEvents(_permanent_events, event_queue);
    AddEvents(_one_time_events, event_queue);
    _one_time_events = "";
    txts.push_back(_txt);
  }
  if (_next) {
    auto more = _next->print(event_queue, parser);
    txts.insert(txts.end(), more.begin(), more.end()); 
  }
  return txts;
}

void Text::ReplaceAt(std::shared_ptr<Text> new_text, int index) {
  // If the next element shall be replaced, replace next with new element
  if (index == 1) {
    if (!_next) {
      throw std::invalid_argument("index out of range for replacing (you may add here)");
    }
    // If the next element is not the last, add following elements to new text
    if (_next->next()) {
      util::Logger()->debug("Text::ReplaceAt: setting last element.");
      new_text->set_next(_next->next());
    }
    // Replace next with new text
    _next = new_text;
  } else if (_next) {
    // Call with index reduced by one.
    _next->ReplaceAt(new_text, --index);
  } else {
    throw std::invalid_argument("index out of range");
  }
}

void Text::InsertAt(std::shared_ptr<Text> new_text, int index) {
  // If the next element shall be replaced, replace next with new element
  if (index == 1) {
    // If the next element is not the last, add following elements to new text
    if (_next) {
      util::Logger()->debug("Text::ReplaceAt: setting last element.");
      new_text->set_next(_next);
    }
    // Replace next with new text
    _next = new_text;
  } else if (_next) {
    // Call with index reduced by one.
    _next->InsertAt(new_text, --index);
  } else {
    throw std::invalid_argument("index out of range");
  }
}

std::shared_ptr<Text> Text::RemoveAt(int index) {
  if (index == 0) {
    if (_next) {
      return _next;
    } 
    return nullptr;
  }
  _next = _next->RemoveAt(--index);
  return shared_from_this();
}

nlohmann::json Text::json() const {
  nlohmann::json j = {{"shared", _shared}, {"txt", _txt}, {"one_time_events", _one_time_events},
    {"permanent_events", _permanent_events}, {"logic", _logic}};
  if (_next) {
    std::vector<nlohmann::json> txts = {j};
    auto next = _next->json();
    if (next.is_array()) {
      txts.insert(txts.end(), next.begin(), next.end()); 
    } else if (next.is_object()) {
      txts.push_back(next);
    }
    return txts;
  } 
  return j;
}

void Text::AddEvents(std::string events, std::string& event_queue) {
  if (events.length() > 0) {
    event_queue += ((event_queue.length() == 0) ? "" : ";") + events;
  }
}
