#ifndef SRC_BUILDER_UTILS_JINJA_HELPERS_H
#define SRC_BUILDER_UTILS_JINJA_HELPERS_H 

#include "builder/utils/reflections.h"
#include "jinja2cpp/reflected_value.h"
#include "jinja2cpp/value.h"
#include <map>
#include <memory>

namespace jhelp {

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
        m.emplace(id, jinja2::Reflect(PtrView{val.get()}));
      } else {
        m.emplace(id, jinja2::Value());
      }
    }
    return m;
  }

}

#endif
