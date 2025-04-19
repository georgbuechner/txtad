//#include "utils/parser/expression_parser.h"
#include "expression_parser.h"
#include "utils/fuzzy_search/fuzzy.h"
#include "utils/utils.h"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <algorithm>
#include <ostream>
#include <string>

std::map<std::string, std::string(*)(const std::string&, const std::string&)> ExpressionParser::opts_ = {
  {">", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) > std::stoi(b)); } },
  {"<", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) < std::stoi(b)); } },
  {">=", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) >= std::stoi(b)); } },
  {"<=", [](const std::string& a, const std::string& b) { return std::to_string(std::stoi(a) <= std::stoi(b)); } },
  {"=", [](const std::string& a, const std::string& b) { return std::to_string(a == b); } },
  {"~", [](const std::string& a, const std::string& b) { return std::to_string(fuzzy::fuzzy(b, a)); } },
  {":", [](const std::string& a, const std::string& b) -> std::string { 
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

ExpressionParser::ExpressionParser() { }

ExpressionParser::ExpressionParser(std::map<std::string, std::string> substitute) {
  substitue_ = substitute;
}

std::string ExpressionParser::evaluate(std::string input) {
  std::cout << input << std::endl;
  auto [pos, opt] = LastOpt(input); 

  // single entry
  if (pos == -1) {
    return util::Strip(input);
  }

  // In brackets
  auto [start, end] = InBrackets(input, pos);
  std::cout << "For " << input << " at " << pos << " got: " << start << ", " << end << std::endl;
  if (start != -1 && end != -1) {
    return evaluate(input.substr(0, start) + evaluate(input.substr(start+1, end-start-1)) 
      + input.substr(end+1, input.length()-end));
  } 

  // Bracket-free equation
  std::string cur = evaluate(input.substr(0, pos));
  std::string next = util::Strip(input.substr(pos+opt.length(), input.length()-(pos + opt.length()-1)));
  std::cout << " - '" << cur << "', '" << opt << "', '" << next << "'" << std::endl;
  return (*opts_[opt])(cur, next);
}

std::pair<int, std::string> ExpressionParser::LastOpt(const std::string& inp) {
  int pos = -1;
  std::string opt = "";
  for (const auto& [cur_opt, _] : opts_) {
    int cur_pos = inp.rfind(cur_opt);
    // std::cout << "- " << cur_pos << ", " << cur_opt << " (stored: " << pos << ", " << opt << ")" << std::endl;
    // std::cout << "- == " << (cur_pos != std::string::npos) << " && (" << (cur_pos-pos > 1) << " || ("
    //   << (abs(cur_pos-pos) <= 1) << " && (" << (cur_opt == "") << " || " << (cur_opt.length() > opt.length()) 
    //   << ")))" << std::endl;

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
  // std::cout << "- => " << opt << std::endl;
  return {pos, opt};
}

std::pair<int, int> ExpressionParser::InBrackets(const std::string& inp, int pos) {
  bool accept = true;
  int end = -1;
  for (int i=0; i<inp.length()-pos; i++) {
    char c = inp[pos+i];
    if (c == '(') 
      accept = false;
    else if (c == ')' && !accept) 
      accept = true;
    else if (c == ')' && accept) {
      end = pos+i;
      break;
    }
  }
  accept = true;
  int start = -1; 
  for (int i=0; i<=pos; i++) {
    char c = inp[pos-i];
    if (c == ')') 
      accept = false;
    else if (c == '(' && !accept) 
      accept = true;
    else if (c == '(' && accept) {
      start = pos-i;
      break;
    }
  }
  return {start, end};
}
