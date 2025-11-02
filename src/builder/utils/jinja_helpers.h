#ifndef SRC_BUILDER_UTILS_JINJA_HELPERS_H
#define SRC_BUILDER_UTILS_JINJA_HELPERS_H 

#include "builder/utils/defines.h"
#include "jinja2cpp/reflected_value.h"
#include "jinja2cpp/value.h"
#include "game/game/game.h"
#include "jinja2cpp/reflected_value.h"
#include "shared/objects/context/context.h"
#include <unordered_map>

#include <map>
#include <memory>

struct GamePtrView {
  const Game* p; // non-owning
};
struct CtxPtrView {
  const Context* p; // non-owning
};


namespace _jinja {
  template<typename T1>
  jinja2::ValuesList SetToVec(const std::set<T1>& set) {
    jinja2::ValuesList vl;
    vl.reserve(set.size());
    for (const auto& it : set) {
      vl.emplace_back(it);
    }
    return vl;
  }

  template<>
  jinja2::ValuesList SetToVec(const std::set<std::pair<std::string, std::string>>& set) {
    jinja2::ValuesList vl;
    vl.reserve(set.size());
    for (const auto& it : set) {
      jinja2::ValuesMap vm;
      vm["first"] = it.first;
      vm["second"] = it.second;
      vl.emplace_back(vm);
    }
    return vl;
  }


  template<typename T1, typename T2>
  jinja2::ValuesList MapKeys(const std::map<T1, T2>& map) {
    jinja2::ValuesList vl;
    vl.reserve(map.size());
    for (const auto& [id, _] : map) {
      vl.emplace_back(id);
    }
    return vl;
  }

  template<typename T1, typename T2>
  jinja2::ValuesMap Map(const std::map<T1, std::shared_ptr<T2>>& map) {
    jinja2::ValuesMap m;
    m.reserve(map.size());
    for (const auto& [id, val] : map) {
      if (val) {
        m.emplace(id, jinja2::Reflect(GamePtrView{val.get()}));
      } else {
        m.emplace(id, jinja2::Value());
      }
    }
    return m;
  }
  template<typename T1>
  jinja2::ValuesMap Map(const std::map<T1, builder::FileType>& map) {
    jinja2::ValuesMap m;
    m.reserve(map.size());
    for (const auto& [id, val] : map) {
      std::cout << id << ", " << val <<std::endl;
      m.emplace(id, builder::FILE_TYPE_MAP.at(val));
    }
    return m;
  }
}

namespace jinja2 {
  template<> struct TypeReflection<GamePtrView> : TypeReflected<GamePtrView> {
    static auto& GetAccessors() {
      static std::unordered_map<std::string, FieldAccessor> acc = {
        {"name", [](const GamePtrView& v){ return (v.p) ? v.p->name() : Value{}; }},
        {"path", [](const GamePtrView& v){ return (v.p) ? v.p->path() : Value{}; }},
        {"contexts", [](const GamePtrView& v){ return (v.p) ? _jinja::MapKeys(v.p->contexts()) : Value{}; }},
        {"description", [](const GamePtrView& v){ return (v.p) ? v.p->builder_settings()._description : Value{}; }},
      };
      return acc;
    }
  };
}

namespace jinja2 {
  template<> struct TypeReflection<CtxPtrView> : TypeReflected<CtxPtrView> {
    static auto& GetAccessors() {
      static std::unordered_map<std::string, FieldAccessor> acc = {
        {"id", [](const CtxPtrView& v){ return (v.p) ? v.p->id() : Value{}; }},
        {"name", [](const CtxPtrView& v){ return (v.p) ? v.p->name() : Value{}; }},
        {"description", [](const CtxPtrView& v){ return (v.p) ? v.p->description() : Value{}; }},
      };
      return acc;
    }
  };
}

#endif
