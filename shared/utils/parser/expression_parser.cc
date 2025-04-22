//#include "utils/parser/expression_parser.h"
#include "expression_parser.h"
#include "utils/fuzzy_search/fuzzy.h"
#include "utils/utils.h"
#include "utils/defines.h"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <algorithm>
#include <ostream>
#include <spdlog/spdlog.h>
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

std::string ExpressionParser::Evaluate(std::string input) {
  spdlog::get(util::LOGGER)->info(fmt::format("EP:Evaluate. START: {}", input));
  return evaluate(EnsureExecutionOrder(input));
}

std::string ExpressionParser::evaluate(std::string input) {
  spdlog::get(util::LOGGER)->debug(fmt::format("EP:Evaluate. {}", input));
  auto [pos, opt] = LastOpt(input); 

  // single entry
  if (pos == -1) {
    return util::Strip(input);
  }

  // In brackets
  auto [start, end] = InBrackets(input, pos);
  spdlog::get(util::LOGGER)->debug(fmt::format("EP:Evaluate. For {}, {} got {}, {}", input, pos, start, end));
  if (start != -1 && end != -1) {
    return evaluate(input.substr(0, start) + evaluate(input.substr(start+1, end-start-1)) 
      + input.substr(end+1, input.length()-end));
  } 

  // Bracket-free equation
  std::string cur = evaluate(input.substr(0, pos));
  std::string next = util::Strip(input.substr(pos+opt.length(), input.length()-(pos + opt.length()-1)));
  spdlog::get(util::LOGGER)->debug(fmt::format(" - '{}', '{}', {}", cur, opt, next));
  return (*opts_[opt])(util::Strip(util::Strip(cur, '('), ')'), 
      util::Strip(util::Strip(next, ')'), '('));
}

std::pair<int, std::string> ExpressionParser::LastOpt(const std::string& inp) {
  int pos = -1;
  std::string opt = "";
  for (const auto& [cur_opt, _] : opts_) {
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
  return {PrevBracket(inp, pos), NextBracket(inp, pos)};
}

int ExpressionParser::NextBracket(const std::string& inp, int pos) {
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
  return end;
}

int ExpressionParser::PrevBracket(const std::string& inp, int pos) {
  bool accept = true;
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
  return start;
}


std::string ExpressionParser::EnsureExecutionOrder(std::string inp) {
  spdlog::get(util::LOGGER)->debug("EP:EnsureExecutionOrder. {}", inp);
  const std::vector<char> priority_operands = {'/', '*'};

  // Check whether priority operands occur in given string
  bool priority_operands_occur = false;
  int num_operands = 0;
  for (const auto& it : priority_operands) {
    if (inp.find(it) != std::string::npos) {
      priority_operands_occur = true;
      break;
    }
  }
  // If not, return empty string
  if (!priority_operands_occur) 
    return inp;

  // Insert brackets to ensure priority f.e. "2+3*5" -> "2+(3*5)"
  std::string modified = "";
  std::string waiting = "";
  bool evaluate_at_next = false;
  int last = 0;
  for (unsigned int i=0; i<inp.length(); i++) {
    const auto& c = inp[i];
    spdlog::get(util::LOGGER)->debug("EP:EnsureExecutionOrder. - {} -> {} (waiting: {}, last: {})", 
        modified, c, waiting, last);
    
    // Handle brackets:
    if (c == '(') {
      auto pos = NextBracket(inp, i+1);
      if (pos != -1) {
        // EnsureExecutionOrder for inside of bracket then add complete content
        // to modified or waiting:
        if (evaluate_at_next)
          waiting += "(" + EnsureExecutionOrder(inp.substr(i+1, pos-i-1)) + ")";
        else
          modified += "(" + EnsureExecutionOrder(inp.substr(i+1, pos-i-1)) + ")";
        i = pos;
        continue;
      }
    }

    // If current char is not (part of) a priority operand, 
    if (std::find(priority_operands.begin(), priority_operands.end(), c) == priority_operands.end()) {
      for (const auto& it : opts_) {
        auto pos = it.first.find(c);
        // if it is, mark current position as 
        if (pos != std::string::npos) {
          if (evaluate_at_next) {
            spdlog::get(util::LOGGER)->debug(fmt::format("EP:EnsureExecutionOrder. -> {} (adding: {})", modified, waiting));
            modified += "(" + waiting + ")";
            evaluate_at_next = false;
          } 
          last = modified.length() + ((pos == 0) ? it.first.length() : it.first.length()-pos); 
          break;
        }
      }
    } else {
      if (evaluate_at_next) {
        spdlog::get(util::LOGGER)->debug(fmt::format("EP:EnsureExecutionOrder. -> {} (adding: {})", modified, waiting));
        waiting = "(" + waiting + ")"; // evaluate(waiting, true);
        spdlog::get(util::LOGGER)->debug(fmt::format("EP:EnsureExecutionOrder. -> {} (new waiting: {})", modified, waiting));
      } else {
        spdlog::get(util::LOGGER)->debug(fmt::format("EP:EnsureExecutionOrder. -> {} (last: {})", modified, last));
        waiting = modified.substr(last, modified.length()-last);
        spdlog::get(util::LOGGER)->debug(fmt::format("EP:EnsureExecutionOrder. => {} (new waiting: {})", modified, waiting));
        modified.erase(modified.begin()+last, modified.end());
        evaluate_at_next = true;
      }
    }

    if (evaluate_at_next) 
      waiting += c;
    else {
      modified +=c;
    }
  }
  if (evaluate_at_next) {
    spdlog::get(util::LOGGER)->debug(fmt::format("EP:EnsureExecutionOrder. -> {} (adding: {})", modified, waiting));
    modified += "(" + waiting + ")";
  }

  spdlog::get(util::LOGGER)->debug(fmt::format("EP:EnsureExecutionOrder. => {}", modified));
  return modified;
}
