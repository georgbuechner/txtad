//#include "utils/parser/expression_parser.h"
#include "expression_parser.h"
#include "utils/fuzzy_search/fuzzy.h"
#include "utils/utils.h"
#include "utils/defines.h"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <algorithm>
#include <memory>
#include <ostream>
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
  {"~", [](const std::string& a, const std::string& b) { return std::to_string(fuzzy::fuzzy(b, a)); } },
  {":", [](const std::string& a, const std::string& b) -> std::string { 
          util::Logger()->debug("EP:InList. {} in {}", a, b);
          const std::string vec = (a.front() != '[') ? "[" + a + "]" : a;
          for (const auto& it : util::Split(vec.substr(1, vec.length()-2), "|")) {
            const std::string e = util::Strip(it);
            for (const auto& elem : util::Split(b.substr(1, b.length()-2), ";")) {
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

ExpressionParser::ExpressionParser(const std::map<std::string, std::string>* substitutes) {
  _substitutes = substitutes;
}

std::string ExpressionParser::Evaluate(std::string input) const {
  util::Logger()->info(fmt::format("EP:Evaluate. START: {}", input));
  auto res = evaluate(EnsureExecutionOrder(input));
  util::Logger()->info(fmt::format("EP:Evaluate. =>: {}", res));
  return res;
}

std::string ExpressionParser::evaluate(std::string input) const {
  util::Logger()->debug(fmt::format("EP:Evaluate. {}", input));
  auto [pos, opt] = LastOpt(input); 

  // If no operand was found, simply return string with whitespaces removed.
  if (pos == -1)
    return StripAndSubstitute(input);

  // Check whether current operand is surrounded by brackets
  auto [start, end] = InBrackets(input, pos);
  // If yes, evaluate brackets first.
  if (start != -1 && end != -1) {
    return evaluate(input.substr(0, start) + evaluate(input.substr(start+1, end-start-1)) 
      + input.substr(end+1, input.length()-end));
  } 

  // Bracket-free equation
  std::string a = evaluate(input.substr(0, pos));
  std::string b = util::Strip(input.substr(pos+opt.length(), input.length()-(pos + opt.length()-1)));

  util::Logger()->debug(fmt::format(" - '{}', '{}', {}", a, opt, b));
  return (*_opts[opt])(StripAndSubstitute(a), StripAndSubstitute(b)); 
}

std::string ExpressionParser::StripAndSubstitute(std::string str) const {
  // strip string from whitespaces and brackets
  str = util::Strip(util::Strip(util::Strip(str), ')'), '(');

  // Check for substitutes
  std::string replaced = "";
  for (int i=0; i<str.length(); i++) {
    // If char is start of substitute, find matching closing bracket
    if (str[i] == '{') {
      int closing = ClosingBracket(str, i+1, '{', '}');
      // Get substitute-name (string inbetween brackets) and check if it exists
      std::string subsitute = str.substr(i+1, closing-(i+1));
      std::string replacement = (_default_subsitutes.count(subsitute) > 0) 
        ? _default_subsitutes.at(subsitute) : (_substitutes->count(subsitute) > 0) 
          ? _substitutes->at(subsitute) : "";
      if (replacement != "") {
        // Add substituted string to replaced string and increase index
        replaced += replacement;
        i = closing;
      }
    } else {
      replaced += str[i]; // add current char
    }
  }
  return replaced;
}


std::pair<int, std::string> ExpressionParser::LastOpt(const std::string& inp) {
  int pos = -1;
  std::string opt = "";
  for (const auto& [cur_opt, _] : _opts) {
    int cur_pos = inp.rfind(cur_opt);
    // If the operand was found, then check 1. if the position is the furthest
    // back (add +1 to the current pass to avoid '=' being further back than
    // '>=') 2. no operand found before, 3. the current operand is longer than
    // the one found sofar.
    if (cur_pos != std::string::npos && (cur_pos-pos > 1 || 
          (abs(cur_pos-pos) <= 1 && (cur_opt == "" || cur_opt.length() > opt.length())))) {
      pos = cur_pos;
      opt = cur_opt;
    }
  }
  return {pos, opt};
}

std::pair<int, int> ExpressionParser::InBrackets(const std::string& inp, int pos) {
  return {OpeningBracket(inp, pos), ClosingBracket(inp, pos)};
}


int ExpressionParser::ClosingBracket(const std::string& inp, int pos, char open, char close) {
  int accept = 0;
  int end = -1;
  for (int i=0; i<inp.length()-pos; i++) {
    char c = inp[pos+i];
    if (c == open) 
      accept++;
    else if (c == close && accept > 0) 
      accept--;
    else if (c == close && accept == 0) {
      end = pos+i;
      break;
    }
  }
  return end;
}

int ExpressionParser::OpeningBracket(const std::string& inp, int pos, char open, char close) {
  int accept = 0;
  int start = -1; 
  for (int i=0; i<=pos; i++) {
    char c = inp[pos-i];
    if (c == close) 
      accept++;
    else if (c == open && accept > 0) 
      accept--;
    else if (c == open && accept == 0) {
      start = pos-i;
      break;
    }
  }
  return start;
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
  int last = 0;
  for (unsigned int i=0; i<inp.length(); i++) {
    const auto& c = inp[i];
    util::Logger()->debug("EP:EnsureExecutionOrder. - {} -> {} (waiting: {}, last: {})", 
        modified, c, waiting, last);
    
    // Handle brackets:
    if (c == '(') {
      auto pos = ClosingBracket(inp, i+1); // get closing bracket
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
      for (const auto& it : _opts) {
        auto pos = it.first.find(c);
        if (pos != std::string::npos) {
          // If waiting is not empty added waiting to modified, surrounded by brackets
          if (waiting != "") {
            util::Logger()->debug(fmt::format("EP:EnsureExecutionOrder. -> {} (adding: {})", modified, waiting));
            modified += "(" + waiting + ")";
            waiting = "";
          } 
          // Set position of last found operand to position of current operand.
          last = modified.length() + ((pos == 0) ? it.first.length() : it.first.length()-pos); 
          break;
        }
      }
    } 
    // If current char is priority-operand start waiting from last-operand to current position. 
    else {
      // If waiting already set, surround with brackets 
      if (waiting != "") {
        util::Logger()->debug(fmt::format("EP:EnsureExecutionOrder. -> {} (adding: {})", modified, waiting));
        waiting = "(" + waiting + ")"; // evaluate(waiting, true);
        util::Logger()->debug(fmt::format("EP:EnsureExecutionOrder. -> {} (new waiting: {})", modified, waiting));
      } 
      // Otherwise, set new waiting (last -> current pos)
      else {
        util::Logger()->debug(fmt::format("EP:EnsureExecutionOrder. -> {} (last: {})", modified, last));
        waiting = modified.substr(last, modified.length()-last);
        util::Logger()->debug(fmt::format("EP:EnsureExecutionOrder. => {} (new waiting: {})", modified, waiting));
        modified.erase(modified.begin()+last, modified.end());
      }
    }

    // Add current char to waiting or modified
    add((waiting != "") ? waiting : modified, fmt::format("{}", c));
  }
  // If waiting is not empty add to modified surrounded by brackets
  if (waiting != "") {
    util::Logger()->debug(fmt::format("EP:EnsureExecutionOrder. -> {} (adding: {})", modified, waiting));
    modified += "(" + waiting + ")";
  }

  util::Logger()->debug(fmt::format("EP:EnsureExecutionOrder. => {}", modified));
  return modified;
}
