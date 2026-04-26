/**
 * @author fux
 */
#ifndef SHARED_UTILS_PARSER_PATTERNPARSER_H_
#define SHARED_UTILS_PARSER_PATTERNPARSER_H_

#include "game/utils/defines.h"
#include <optional>
#include <string>
#include <variant>
namespace pattern {

  constexpr std::string REPLACE_CTX = " -> ";
  struct ReplaceCtx {
    const std::string original_ctx; 
    const std::string new_ctx; 
  };
  ///< Pattern: {original_ctx} -> {new_ctx})
  std::optional<ReplaceCtx> replace_ctx(const std::string& inp);

  constexpr std::string SET_CTX_NAME = " = ";
  struct SetCtxName {
    const std::string ctx_id; 
    const std::string value; 
  };
  ///< Pattern: {attribute-id} = {value})
  std::optional<SetCtxName> set_ctx_name(const std::string& inp);

  struct SetAttribute { 
    const std::string ctx_id;
    const std::string attribute_id; 
    const std::string opt;
    const std::string expression;
  };
  std::optional<SetAttribute> set_attribute(const std::string& inp);

  struct CtxMemberAccess {
    const std::string ctx_id; 
    const enum Kind { VARIABLE, ATTRIBUTE } member_type;  
    const std::string key;
  };
  std::optional<CtxMemberAccess> member_access(const std::string& inp);

  size_t find(const std::string& inp, const std::string& sub_str); 
  size_t rfind(const std::string& inp, const std::string& sub_str); 
}

#endif
