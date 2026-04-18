//#include "utils/parser/expression_parser.h"
#include "expression_parser.h"
#include "game/utils/defines.h"
#include "shared/utils/fuzzy_search/fuzzy.h"
#include "shared/utils/utils.h"
#include <algorithm>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>

const std::map<std::string, std::string> ExpressionParser::_default_subsitutes = {{"no_match", "0"}, 
  {"direct", "1"}, {"starts_with", "2"}, {"contains", "3"}, {"fuzzy", "4"}};

std::map<std::string, std::string(*)(const std::string&, const std::string&)> ExpressionParser::_opts = {
  {">", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) > std::stoi(b)); } },
  {"<", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) < std::stoi(b)); } },
  {">=", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) >= std::stoi(b)); } },
  {"<=", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) <= std::stoi(b)); } },
  {"=", [](const std::string& a, const std::string& b) { return std::to_string(a == b); } },
  {"!=", [](const std::string& a, const std::string& b) { return std::to_string(a != b); } },
  {"~", [](const std::string& a, const std::string& b) { return std::to_string(fuzzy::fuzzy(b, a)); } },
  {"~1", [](const std::string& a, const std::string& b) { 
            return std::to_string(fuzzy::fuzzy(b, a) == fuzzy::FuzzyMatch::DIRECT); } },
  {"~2", [](const std::string& a, const std::string& b) { 
            return std::to_string(fuzzy::fuzzy(b, a) == fuzzy::FuzzyMatch::STARTS_WITH); } },
  {"~3", [](const std::string& a, const std::string& b) { 
            return std::to_string(fuzzy::fuzzy(b, a) == fuzzy::FuzzyMatch::CONTAINS); } },
  {"~4", [](const std::string& a, const std::string& b) { 
            return std::to_string(fuzzy::fuzzy(b, a) == fuzzy::FuzzyMatch::FUZZY); } },
  {":", [](const std::string& a, const std::string& b) -> std::string { 
          const std::string vec = (a.front() != '[') ? a : a.substr(1, a.length()-2);
          const std::string vec_b = (b[1] == '\'' && b[b.length()-2] == '\'') 
            ? b.substr(2, b.length()-4) : b.substr(1, b.length()-2);
          std::string sep = (vec_b.find(";") != std::string::npos) ? ";" : ",";
          for (const auto& it : util::Split(vec, "|")) {
            const std::string e = util::Strip(it);
            for (const auto& elem : util::Split(vec_b, sep)) {
              if (util::Strip(elem) == e) return "1";
            }
          }
          return "0";
        } },
  {"~:", [](const std::string& a, const std::string& b) -> std::string { 
          std::string res_vec = "";
          for (const auto& elem : util::Split(b.substr(1, b.length()-1), ";")) {
            auto res = fuzzy::fuzzy(util::Strip(elem), a);
            if (res != fuzzy::FuzzyMatch::NO_MATCH) 
              res_vec += std::to_string(res) + ";";
          }
          if (res_vec.back() == ';') 
            res_vec.pop_back();
          return "[" + res_vec + "]";
        } },
  {"+", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) + std::stoi(b)); } },
  {"-", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) - std::stoi(b)); } },
  {"*", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) * std::stoi(b)); } },
  {"/", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) / std::stoi(b)); } },
  {"||", [](const std::string& a, const std::string& b) { return std::to_string(a == "1" || b == "1"); } },
  {"&&", [](const std::string& a, const std::string& b) { return std::to_string(a == "1" && b == "1"); } },
};

ExpressionParser::ExpressionParser() { 
  _substitute_fn = [](const auto& str) -> std::string { 
    util::Logger()->debug("CALLED default substitute-fn with {}", str);
    return ""; 
  };
}
ExpressionParser::ExpressionParser(const SubstituteFN& fn) {
  _substitute_fn = [fn](const std::string& str) {
    util::Logger()->debug("CALLED custom substitute-fn with {}", str);
    return fn(str);
  };
}

std::string ExpressionParser::Evaluate(std::string input) const {
  util::Logger()->info("EP:Evaluate. START: {}", input);

  // Check for substitutes
  std::string replaced = "";
  for (int i=0; i<input.length(); i++) {
    // If char is start of substitute, find matching closing bracket
    if (input[i] == '{') {
      int closing = util::ClosingBracket(input, i+1, '{', '}');
      // Get substitute-name (string inbetween brackets) and check if it exists
      std::string subsitute = input.substr(i+1, closing-(i+1));
      util::Logger()->info("FOUND SUBSTITUE: {}", subsitute);
      std::string replacement = "";
      if (_default_subsitutes.count(subsitute) > 0) {
        replacement = _default_subsitutes.at(subsitute);
      } else {
        replacement = _substitute_fn(subsitute);
        if (replacement == "") {
          replacement = "''";
        } else if (replacement == txtad::NO_REPLACEMENT) {
          util::Logger()->error("No subsitute found for: {}", subsitute);
        } else {
          replacement = "\'" + replacement + "\'";
        }
      } 
      if (replacement != "") {
        // Add substituted string to replaced string and increase index
        replaced += replacement;
        i = closing;
      }
    } else {
      replaced += input[i]; // add current char
    }
  }

  auto res = evaluate(EnsureExecutionOrder(replaced));

  util::Logger()->info("EP:Evaluate. =>: {}", res);
  return res;
}

std::string ExpressionParser::evaluate(std::string input) const {
  util::Logger()->debug("EP:Evaluate. {}", input);
  auto [pos, opt] = LastOpt(input); 

  // If no operand was found, simply return string with whitespaces removed.
  if (pos == -1)
    return StripAndSubstitute(input);

  // Check whether current operand is surrounded by brackets
  auto [start, end] = util::InBrackets(input, pos);
  // If yes, evaluate brackets first.
  if (start != -1 && end != -1) {
    return evaluate(input.substr(0, start) + evaluate(input.substr(start+1, end-start-1)) 
      + input.substr(end+1, input.length()-end));
  } 

  // Bracket-free equation
  std::string a = evaluate(input.substr(0, pos));
  std::string b = util::Strip(input.substr(pos+opt.length(), input.length()-(pos + opt.length()-1)));

  util::Logger()->debug(" - '{}', '{}', {}", a, opt, b);
  return (*_opts[opt])(StripAndSubstitute(a), StripAndSubstitute(b)); 
}

std::string ExpressionParser::StripAndSubstitute(std::string str) const {
  // strip string from whitespaces and brackets
  return util::Strip(util::Strip(util::Strip(util::Strip(str), ')'), '('), '\'');
}


std::pair<int, std::string> ExpressionParser::LastOpt(const std::string& inp) {
  bool escaped = false;
  for (int i=inp.length()-1; i>0; i--) {
    const char& c = inp[i];
    if (c == '\'') {
      escaped = !escaped;
    }
    if (escaped) {
      continue;
    }

    // Check current char is part of operator
    if (auto remaining_len = RPartOfOpt(i, inp)) {
      const std::string opt = std::string(1, c);
      // Check direct match
      if (remaining_len == 1) {
        return {i, opt};
      } else {
        return {i-1, std::string(1, inp[i-1]) + opt};
      }
    }
  }
  return {-1, ""};
}

std::optional<short> ExpressionParser::PartOfOpt(int i, const std::string& inp) {
  std::string a = std::string(1, inp[i]);
  if (_opts.contains(a))
    return std::optional(1);
  if (i > 0 && _opts.contains(a + std::string(1, inp[i-1])))
    return std::optional(2);
  if (i < inp.size()-1 && _opts.contains(std::string(1, inp[i+1]) + a)) 
    return std::optional(1);
  return std::nullopt;
}

std::optional<short> ExpressionParser::RPartOfOpt(int i, const std::string& inp) {
  std::string a = std::string(1, inp[i]);
  if (i > 0 && _opts.contains(std::string(1, inp[i-1]) + a))
    return std::optional(2);
  if (_opts.contains(a))
    return std::optional(1);
  return std::nullopt;
}


std::string ExpressionParser::EnsureExecutionOrder(std::string inp) {
  util::Logger()->debug("EP:EnsureExecutionOrder. {}", inp);
  const std::vector<char> priority_operands = {'/', '*'};

  // Check whether priority operands occur in given string
  if (std::find_if(priority_operands.begin(), priority_operands.end(), [&](char c) {
        return inp.find(c) != std::string::npos; }) == priority_operands.end())
    return inp;

  // Define lambda for adding to modified or waiting 
  auto add = [](std::string& cur, const std::string& str) { cur += str; };

  // Insert brackets to ensure priority f.e. "2+3*5" -> "2+(3*5)"
  std::string modified = "";
  std::string waiting = "";
  bool escaped = false;
  int last = 0;
  for (unsigned int i=0; i<inp.length(); i++) {
    const auto& c = inp[i];

    if (c == '\'') {
      escaped = !escaped; 
    }

    if (!escaped) {
      // Handle brackets:
      if (c == '(') {
        auto pos = util::ClosingBracket(inp, i+1); // get closing bracket
        if (pos != -1) {
          // Recursive call for inside of bracket and add complete content to modified/waiting:
          std::string inside_bracket = "(" + EnsureExecutionOrder(inp.substr(i+1, pos-i-1)) + ")";
          add((waiting != "") ? waiting : modified, inside_bracket);
          i = pos; // increase position to closing bracket
          continue;
        }
      }

      // If current char is not (part of) a priority operand:
      if (std::find(priority_operands.begin(), priority_operands.end(), c) == priority_operands.end()) {
        if (auto remaining_len = PartOfOpt(i, inp)) {
          // If waiting is not empty, add (waiting) to modified 
          if (waiting != "") {
            util::Logger()->debug("EP:EnsureExecutionOrder. -> {} (adding: {})", modified, waiting);
            modified += "(" + waiting + ")";
            waiting = "";
          } 
          // Set position of last found operand to position of current operand.
          last = modified.length() + *remaining_len; 
        }
      } 
      // If current char is priority-operand start waiting from last-operand to current position. 
      else {
        // If waiting already set, surround with brackets 
        if (waiting != "") {
          util::Logger()->debug("EP:EnsureExecutionOrder. -> {} (adding: {})", modified, waiting);
          waiting = "(" + waiting + ")"; // evaluate(waiting, true);
          util::Logger()->debug("EP:EnsureExecutionOrder. -> {} (new waiting: {})", modified, waiting);
        } 
        // Otherwise, set new waiting (last -> current pos)
        else {
          util::Logger()->debug("EP:EnsureExecutionOrder. -> {} (last: {})", modified, last);
          waiting = modified.substr(last, modified.length()-last);
          util::Logger()->debug("EP:EnsureExecutionOrder. => {} (new waiting: {})", modified, waiting);
          modified.erase(modified.begin()+last, modified.end());
        }
      }
    }

    // Add current char to waiting or modified
    add((waiting != "") ? waiting : modified, fmt::format("{}", c));
  }
  // If waiting is not empty add to modified surrounded by brackets
  if (waiting != "") {
    util::Logger()->debug("EP:EnsureExecutionOrder. -> {} (adding: {})", modified, waiting);
    modified += "(" + waiting + ")";
  }

  util::Logger()->debug("EP:EnsureExecutionOrder. => {}", modified);
  return modified;
}
