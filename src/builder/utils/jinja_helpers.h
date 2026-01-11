#ifndef SRC_BUILDER_UTILS_JINJA_HELPERS_H
#define SRC_BUILDER_UTILS_JINJA_HELPERS_H 

#include "builder/utils/defines.h"
#include "jinja2cpp/reflected_value.h"
#include "jinja2cpp/value.h"
#include "game/game/game.h"
#include "jinja2cpp/reflected_value.h"
#include "shared/objects/context/context.h"
#include "shared/objects/settings/settings.h"
#include "shared/objects/tests/test.h"
#include "shared/objects/tests/test_case.h"
#include "shared/utils/eventmanager/listener.h"
#include "shared/utils/utils.h"
#include <unordered_map>

#include <map>
#include <memory>

template<typename T1>
struct PtrView {
  const T1* ptr;
};

namespace _jinja {

  template<typename T1>
  jinja2::ValuesList Vec(const std::vector<T1>& vec) {
    jinja2::ValuesList vl;
    vl.reserve(vec.size());
    for (const auto& it : vec) {
      vl.emplace_back(jinja2::Reflect(it));
    }
    return vl;
  }

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
        m.emplace(id, jinja2::Reflect(PtrView<T2>{val.get()}));
      } else {
        m.emplace(id, jinja2::Value());
      }
    }
    return m;
  }

  template<typename T1, typename T2>
  jinja2::ValuesMap Map(const std::map<T1, T2>& map) {
    jinja2::ValuesMap m;
    m.reserve(map.size());
    for (const auto& [id, val] : map) {
      m.emplace(id, val);
    }
    return m;
  }

  template<typename T1>
  jinja2::ValuesMap Map(const std::map<T1, builder::FileType>& map) {
    jinja2::ValuesMap m;
    m.reserve(map.size());
    for (const auto& [id, val] : map) {
      m.emplace(id, builder::FILE_TYPE_MAP.at(val));
    }
    return m;
  }
}

namespace jinja2 {
  namespace detail {
    template<>
    struct Reflector<PtrView<Text>, void> {
      static Value Create(const PtrView<Text>& v) {
        jinja2::ValuesList list;
        if (v.ptr) {
          auto* t = v.ptr;
          while (t) {
            list.emplace_back(jinja2::Reflect(*t));  // Text stays element-like
            t = t->next();
          }
        }
        return jinja2::Value{std::move(list)};
      }
    };
  }
}

namespace jinja2 {
  template<> struct TypeReflection<PtrView<Game>> : TypeReflected<PtrView<Game>> {
    static auto& GetAccessors() {
      static std::unordered_map<std::string, FieldAccessor> acc = {
        {"settings", [](const PtrView<Game>& v){ return (v.ptr) ? jinja2::Reflect(v.ptr->settings()) : Value{}; }},
        {"name", [](const PtrView<Game>& v){ return (v.ptr) ? v.ptr->name() : Value{}; }},
        {"path", [](const PtrView<Game>& v){ return (v.ptr) ? v.ptr->path() : Value{}; }},
        {"contexts", [](const PtrView<Game>& v){ return (v.ptr) ? _jinja::Map(v.ptr->contexts()) : Value{}; }},
        {"texts", [](const PtrView<Game>& v){ return (v.ptr) ? _jinja::Map(v.ptr->texts()) : Value{}; }},
        {"description", [](const PtrView<Game>& v){ return (v.ptr) ? v.ptr->builder_settings()._description : Value{}; }},
        {"running", [](const PtrView<Game>& v){ return (v.ptr) ? v.ptr->running() : Value{}; }},
      };
      return acc;
    }
  };

  template<> struct TypeReflection<Settings> : TypeReflected<Settings> {
    static auto& GetAccessors() {
      static std::unordered_map<std::string, FieldAccessor> acc = {
        {"initial_events", [](const Settings& settings) { return settings.initial_events(); }},
        {"initial_contexts", [](const Settings& settings) { return _jinja::Vec(settings.initial_ctx_ids()); }},
      };
      return acc;
    }
   };


  template<> struct TypeReflection<Text> : TypeReflected<Text> {
    static auto& GetAccessors() {
      static std::unordered_map<std::string, FieldAccessor> acc = {
        {"shared", [](const Text& txt) { return txt.shared(); }},
        {"txt", [](const Text& txt) { return txt.txt(); }},
        {"logic", [](const Text& txt) { return txt.logic(); }},
        {"one_time_events", [](const Text& txt) { return txt.one_time_events(); }},
        {"permanent_events", [](const Text& txt) { return txt.permanent_events(); }},
      };
      return acc;
    }
   };

  template<> struct TypeReflection<PtrView<Listener>> : TypeReflected<PtrView<Listener>> {
    static auto& GetAccessors() {
      static std::unordered_map<std::string, FieldAccessor> acc {
        {"id", [](const PtrView<Listener> v) { return (v.ptr) ? v.ptr->id() : Value{}; }},
        {"event", [](const PtrView<Listener> v) { return (v.ptr) ? v.ptr->event() : Value{}; }}, 
        {"permeable", [](const PtrView<Listener> v) { return (v.ptr) ? v.ptr->permeable() : Value{}; }}, 
        {"arguments", [](const PtrView<Listener> v) { return (v.ptr)
            ? v.ptr->original_json().at("arguments").get<std::string>() 
            : Value{}; }}, 
        {"logic", [](const PtrView<Listener> v) { return (v.ptr) ? v.ptr->original_json().value("logic", "") : Value{}; }}, 
        {"ctx", [](const PtrView<Listener> v) { return (v.ptr) ? v.ptr->ctx_id() : Value{}; }}, 
        {"exec", [](const PtrView<Listener> v) { return (v.ptr) ? v.ptr->original_json().value("exec", false) : Value{}; }}, 
      };
      return acc;
    }
  };


  template<> struct TypeReflection<PtrView<Context>> : TypeReflected<PtrView<Context>> {
    static auto& GetAccessors() {
      static std::unordered_map<std::string, FieldAccessor> acc = {
        {"id", [](const PtrView<Context>& v) { return (v.ptr) ? v.ptr->id() : Value{}; }},
        {"name", [](const PtrView<Context>& v) { return (v.ptr) ? v.ptr->name() : Value{}; }},
        {"entry", [](const PtrView<Context>& v) { return (v.ptr) ? v.ptr->entry_condition_pattern() : Value{}; }},
        {"priority", [](const PtrView<Context>& v) { return (v.ptr) ? v.ptr->priority() : Value{}; }},
        {"permeable", [](const PtrView<Context>& v) { return (v.ptr) ? v.ptr->permeable() : Value{}; }},
        {"shared", [](const PtrView<Context>& v) { return (v.ptr) ? v.ptr->shared() : Value{}; }},
        {"description", [](const PtrView<Context>& v) { return (v.ptr) ? jinja2::Reflect(PtrView<Text>{&v.ptr->description()}) : Value{}; }},
        {"attributes", [](const PtrView<Context>& v) { return (v.ptr) ? _jinja::Map(v.ptr->attributes()) : Value{}; }},
        {"listeners", [](const PtrView<Context>& v) { return (v.ptr) ? _jinja::Map(v.ptr->listeners()) : Value{}; }},
      };
      return acc;
    }
  };

  template<> struct TypeReflection<Test> : TypeReflected<Test> {
    static auto& GetAccessors() {
      static std::unordered_map<std::string, FieldAccessor> acc = {
        {"cmd", [](const Test& t) { return t.cmd(); }},
        {"result", [](const Test& t) { return t.result(); }},
        {"checks", [](const Test& t) { return util::Join(t.checks(), ";"); }}
      };
      return acc;
    }
  };
  template<> struct TypeReflection<TestCase> : TypeReflected<TestCase> {
    static auto& GetAccessors() {
      static std::unordered_map<std::string, FieldAccessor> acc = {
        {"desc", [](const TestCase& t) { return jinja2::Value(t.desc()); }},
        {"tests", [](const TestCase& t) { return _jinja::Vec(t.tests()); }}
      };
      return acc;
    }
  };

}

#endif
