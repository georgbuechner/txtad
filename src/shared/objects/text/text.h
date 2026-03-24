#ifndef SRC_SHARED_OBJECTS_TEXT_H_
#define SRC_SHARED_OBJECTS_TEXT_H_

#include "shared/utils/parser/expression_parser.h"
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

class Text : public std::enable_shared_from_this<Text> {
  public: 

    Text(std::string txt, std::string one_time_events, std::string permanent_events, 
        bool shared=true, std::string logic="", Text* next=nullptr);

    Text(std::string txt);

    /**
     * Parses text from json, recursively creating next elements 
     * @param[in] json 
     * @param[in] ctx_id (optional. If present applies this-replacement for
     * events)
     */
    Text(nlohmann::json json, std::string ctx_id="");
    ~Text();

    // getter 
    bool shared() const;
    std::string txt() const;
    std::string one_time_events() const;
    std::string permanent_events() const;
    std::string logic() const;
    std::shared_ptr<Text> next() const;

    // setter 
    void set_next(std::shared_ptr<Text> txt);

    // methods
    std::vector<std::string> print(std::string& event_queue, const ExpressionParser& parser);

    /**
     * Replaces the text at given index. 
     * @param[in] new_text 
     * @param[in] index (>= 1)
     */
    void ReplaceAt(std::shared_ptr<Text> new_text, int index);

    /**
     * Inserts text at the given index 
     * @param[in] new_text 
     * @param[in] index (>= 1)
     */
    void InsertAt(std::shared_ptr<Text> new_text, int index);

    /**
     * Removes text element at the give index 
     * @param[in] index (>= 0)
     */
    std::shared_ptr<Text> RemoveAt(int index);

    /** 
     * Converts text to json
     */
    nlohmann::json json() const;

  private:
    bool _shared;
    std::string _txt;
    std::string _one_time_events;  ///< events only thrown the first time the text is printed
    std::string _permanent_events;   ///< events thrown every time the text is printed
    std::string _logic; ///< logic condition checked before printing
    std::shared_ptr<Text> _next; ///< next text to be printed
  
    static void AddEvents(std::string events, std::string& event_queue);
};

#endif
