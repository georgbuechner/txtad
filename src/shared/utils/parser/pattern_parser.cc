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
    auto p = find(inp, it);
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
  pos = rfind(tmp_attribute, ".");
  if (pos == std::string::npos) {
    util::Logger()->warn("pattern::set_attribute. Invalid attribute id! {}", tmp_attribute);
    return std::nullopt;
  }
  std::string ctx_id = tmp_attribute.substr(0, pos);
  std::string attribute_id = util::Strip(tmp_attribute.substr(pos+1));

  return std::optional(SetAttribute{ctx_id, attribute_id, opt, expression});
}

std::optional<pattern::CtxMemberAccess> pattern::member_access(const std::string& inp) {
  auto pos_a = find(inp, ".");
  auto pos_v = find(inp, "->");
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

size_t pattern::find(const std::string& inp, const std::string& sub_str) {
  if (sub_str.empty()) {
    return 0;
  }
  if (sub_str.size() > inp.size()) {
    return std::string::npos;
  }

  int bracket_depth = 0;
  for (unsigned int i=0; i<inp.size(); i++) {
    if (inp[i] == '[') {
      bracket_depth++;
    } else if (bracket_depth > 0 && inp[i] == ']') {
      bracket_depth--;
    } 

    if (bracket_depth == 0 && i + sub_str.size() <= inp.size()) {
      if (inp.compare(i, sub_str.size(), sub_str) == 0) {
        return i;
      }
    } else if (bracket_depth == 0) {
      break;
    }
  }
  return std::string::npos;
}

size_t pattern::rfind(const std::string& inp, const std::string& sub_str) {
  if (sub_str.empty()) {
    return inp.size();
  }
  if (sub_str.size() > inp.size()) {
    return std::string::npos;
  }

  int bracket_depth = 0;
  for (unsigned int i=inp.size()-1; i>= 0; i--) {
    if (inp[i] == '[') {
      bracket_depth++;
    } else if (bracket_depth > 0 && inp[i] == ']') {
      bracket_depth--;
    } 

    if (bracket_depth == 0 && i + sub_str.size() <= inp.size()) {
      if (inp.compare(i, sub_str.size(), sub_str) == 0) {
        return i;
      }
    } else if (bracket_depth == 0) {
      break;
    }
  }
  return std::string::npos;
}
