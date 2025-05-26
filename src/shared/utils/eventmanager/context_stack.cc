#include "context_stack.h"

ContextStack::ContextStack() {};

// methods
bool ContextStack::insert(std::shared_ptr<Context> context) {
  if (_contexts.count(context->id()) > 0) {
    util::Logger()->warn(fmt::format("ContextStack::Add. Context {} already exists.", context->id()));
    return false;
  }
  _contexts.emplace(context->id(), context);
  // If lowest priority, insert into back
  if (_sorted_contexts.empty()) {
    _sorted_contexts.push_back(context);
  }
  else if (_sorted_contexts.back()->priority() >= context->priority()) {
    _sorted_contexts.push_back(context);
  }
  else {
    for (auto it = _sorted_contexts.begin(); it != _sorted_contexts.end(); ++it) {
      if ((*it)->priority() < context->priority()) {
        _sorted_contexts.insert(it, context);
        break;
      }
    }
  }
  return true;
}

bool ContextStack::erase(const std::string& id) {
  if (_contexts.count(id) == 0) {
    util::Logger()->warn(fmt::format("ContextStack::Remove. Context {} not found.", id));
    return false;
  }
  _contexts.erase(id);
  auto it = std::find_if(_sorted_contexts.begin(), _sorted_contexts.end(), [id](const auto& it) {
    return it->id() == id; });
  if (it != _sorted_contexts.end()) {
    _sorted_contexts.erase(it);
  } else {
    util::Logger()->error(fmt::format("ContextStack::Remove. Context {} removed from map but was not found in vector.", id));
  }
  return true;
}

std::shared_ptr<Context> ContextStack::get(const std::string& id) {
  const auto& it = _contexts.find(id);
  if (it != _contexts.end())
    return it->second;
  util::Logger()->warn(fmt::format("ContextStack::Get. Context {} not found.", id));
  return nullptr;
}

std::vector<std::string> ContextStack::GetOrder() {
  std::vector<std::string> sorted_context_ids;
  std::transform(_sorted_contexts.begin(), _sorted_contexts.end(), std::back_inserter(sorted_context_ids),
        [](const auto& it) { return it->id(); });
  return sorted_context_ids;
}

void ContextStack::TakeEvent(std::string event, const ExpressionParser& parser) {
  for (const auto& it : _sorted_contexts) {
    it->event_manager()->TakeEvent(event, parser);
  }
}

