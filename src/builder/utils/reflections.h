#ifndef SRC_BUILDER_UTILS_REFLECTIONS_H
#define SRC_BUILDER_UTILS_REFLECTIONS_H 

#include "game/game/game.h"
#include "jinja2cpp/reflected_value.h"
#include <unordered_map>

// A lightweight, copyable view of Game
struct PtrView {
  const Game* p; // non-owning
};

namespace jinja2 {
  template<> struct TypeReflection<PtrView> : TypeReflected<PtrView> {
    static auto& GetAccessors() {
      static std::unordered_map<std::string, FieldAccessor> acc = {
        {"name", [](const PtrView& v){ return (v.p) ? v.p->name() : Value{}; }},
        {"path", [](const PtrView& v){ return (v.p) ? v.p->path() : Value{}; }},
      };
      return acc;
    }
  };

  // template<>
  // struct TypeReflection<std::shared_ptr<Game>> : TypeReflected<std::shared_ptr<Game>> {
  //   static auto& GetAccessors() {
  //     using Ptr = std::shared_ptr<Game>;
  //     static std::unordered_map<std::string, FieldAccessor> acc = {
  //       {"name", [](const Ptr& g){ return g ? g->name() : ""; }},
  //       {"path", [](const Ptr& g){ return g ? g->path() : ""; }},
  //     };
  //     return acc;
  //   }
  // };
}

#endif
