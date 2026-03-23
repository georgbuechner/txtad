#include "shared/utils/parser/pattern_parser.h"
#include "shared/utils/utils.h"
#include <optional>
#include <string>

std::optional<pattern::ReplaceCtx> pattern::replace_ctx(const std::string& inp) {
  auto parts = util::Split(inp, REPLACE_CTX);
  if (parts.size() != 2) {
    util::Logger()->info("pattern::replace_ctx: Invalid input \"{}\" for pattern \"(.*) -> (.*)\"", inp);
    return std::nullopt;
  }
  return std::optional(ReplaceCtx{parts[0], parts[1]});
}

std::optional<pattern::SetCtxName> pattern::set_ctx_name(const std::string& inp) {
  auto parts = util::Split(inp, SET_CTX_NAME);
  if (parts.size() != 2) {
    util::Logger()->info("pattern::replace_ctx: Invalid input \"{}\" for pattern \"(.*) = (.*)\"", inp);
    return std::nullopt;
  }
  return std::optional(SetCtxName{parts[0], parts[1]});
}

std::optional<pattern::SetAttribute> pattern::set_attribute(const std::string & inp) {
  static const std::vector<std::string> opts = {"+=", "-=", "++", "--", "*=", "/=", "="}; 
  // Find operator
  std::string opt;
  int pos = std::string::npos;
  for (const auto& it : opts) {
    auto p = inp.find(it);
    if (p != std::string::npos) {
      opt = it;
      pos = p;
      break;
    }
  }
  if (pos == std::string::npos) {
    util::Logger()->warn("pattern::set_attribute: no operator found: {}", inp);
    return std::nullopt;
  }
  // Split input into attribute and expression used to set attribute
  std::string tmp_attribute = inp.substr(0, pos);
  std::string expression = inp.substr(pos+opt.length()); 

  util::Logger()->debug("pattern::set_attribute: attribute: {}, opt: {}, expression: {}", tmp_attribute, opt, expression);

  // Split attribute into ctx-id and attribute-id
  pos = tmp_attribute.rfind(".");
  if (pos == std::string::npos) {
    util::Logger()->warn("pattern::set_attribute. Invalid attribute id! {}", tmp_attribute);
    return std::nullopt;
  }
  std::string ctx_id = tmp_attribute.substr(0, pos);
  std::string attribute_id = util::Strip(tmp_attribute.substr(pos+1));

  return std::optional(SetAttribute{ctx_id, attribute_id, opt, expression});
}

std::optional<pattern::CtxMemberAccess> pattern::member_access(const std::string& inp) {
  auto pos_a = inp.find(".");
  auto pos_v = inp.find("->");
  size_t pos = -1;
  short len = 0;
  pattern::CtxMemberAccess::Kind member_type;

  if (pos_a != std::string::npos && (pos_v == std::string::npos || pos_a < pos_v)) {
    pos = pos_a; 
    len = 1;
    member_type = pattern::CtxMemberAccess::ATTRIBUTE;
  } else if (pos_v != std::string::npos) {
    pos = pos_v; 
    len = 2;
    member_type = pattern::CtxMemberAccess::VARIABLE;
  } else {
    return std::nullopt;
  }
  std::string ctx_id = inp.substr(0, pos);
  std::string key = inp.substr(pos+len, inp.size()-len);
  return std::optional(pattern::CtxMemberAccess{ctx_id, member_type, key});
}
