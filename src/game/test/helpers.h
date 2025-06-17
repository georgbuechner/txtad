#ifndef GAME_TEST_HELPERS_H
#define GAME_TEST_HELPERS_H

#include "shared/utils/parser/expression_parser.h"
#include <map>
#include <string>

namespace helpers {
  void SetAttribute(std::map<std::string, std::string>& attributes, std::string inp, const ExpressionParser& parser);
}

#endif
