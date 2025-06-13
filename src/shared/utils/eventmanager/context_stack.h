#ifndef SRC_UTILS_CONTEXT_STACK_H
#define SRC_UTILS_CONTEXT_STACK_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "shared/objects/context/context.h"
#include "shared/utils/parser/expression_parser.h"

class ContextStack {
  public: 
    ContextStack();

    // methods
    bool exists(const std::string& id) const;
    bool insert(std::shared_ptr<Context> context);
    bool erase(const std::string& id);
    std::shared_ptr<Context> get(const std::string& id);
    std::vector<std::shared_ptr<Context>> find(const std::string& id_part);

    std::vector<std::string> GetOrder();
    void TakeEvents(std::string& events, const ExpressionParser& parser);
    void TakeEvent(const std::string& event, const ExpressionParser& parser);

  private: 
    std::map<std::string, std::shared_ptr<Context>> _contexts;
    std::vector<std::shared_ptr<Context>> _sorted_contexts;
};

#endif
