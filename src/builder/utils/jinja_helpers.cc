
#include "builder/utils/jinja_helpers.h"
#include <stdexcept>

_jinja::Env::Env(std::string template_path) : _fs{template_path} {
  _env.AddFilesystemHandler("", _fs);

  _env.AddGlobal("NEW_LISTENER", builder::NEW_LISTENER);

  _env.AddGlobal("starts_with", jinja2::UserCallable(
      [](auto& params)->jinja2::Value {
        return params["s1"].asString().find(params["s2"].asString()) == 0;
      }, std::vector<jinja2::ArgInfo>({jinja2::ArgInfo{"s1"}, jinja2::ArgInfo{"s2"}})
    ));

  _env.AddGlobal("contains", jinja2::UserCallable(
      [](auto& params)->jinja2::Value {
        return params["s"].asString().find(params["sub"].asString()) != std::string::npos;
      }, std::vector<jinja2::ArgInfo>({jinja2::ArgInfo{"s"}, jinja2::ArgInfo{"sub"}})
    ));

  _env.AddGlobal("safe", jinja2::UserCallable(
      [](auto& params)->jinja2::Value {
        const std::string str = params["s"].asString();
        return util::ReplaceAll(util::ReplaceAll(str, ">", "&gt;"), "<", "&lt;");
      }, std::vector<jinja2::ArgInfo>({jinja2::ArgInfo{"s"}})
    ));

  _env.AddGlobal("is_child", jinja2::UserCallable(
      [](auto& params)->jinja2::Value {
        const std::string path = params["path"].asString();
        const std::string cur = params["cur"].asString();
        if (path.find(cur) == 0) {
          return path.substr(cur.length()).find("/") == std::string::npos;
        }
        return false;
      }, std::vector<jinja2::ArgInfo>({jinja2::ArgInfo{"path"}, jinja2::ArgInfo{"cur"}})
    ));

  _env.AddGlobal("default_attributes", jinja2::UserCallable(
      [](auto& params)->jinja2::ValuesMap {
        return jinja2::ValuesMap({{"", ""}});
      }, std::vector<jinja2::ArgInfo>()
    ));

  _env.AddGlobal("default_test_cases", jinja2::UserCallable(
      [](auto& params)->jinja2::ValuesList {
        std::vector<TestCase> vec = { TestCase() };
        return _jinja::Vec(vec);
      }, std::vector<jinja2::ArgInfo>()
    ));

}

// methods
std::string _jinja::Env::LoadTemplate(std::string page, const jinja2::ValuesMap& params) {
  auto tpl = _env.LoadTemplate(page);
  if (tpl) { 
    if (auto rendered = tpl->RenderAsString(params)) {
      return *rendered;
    } else {
      throw std::runtime_error("Template load error: " + rendered.error().ToString());
    }
  } else {
    throw std::runtime_error("Template load error: " + tpl.error().ToString());
  }
}

template<>
jinja2::ValuesList _jinja::SetToVec(const std::set<std::pair<std::string, std::string>>& set) {
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

