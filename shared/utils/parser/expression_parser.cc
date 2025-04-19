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
            const std::string e = DeleteWhitespaces(it);
            for (const auto& elem : util::Split(b.substr(1, b.length()-2), ";")) {
              if (DeleteWhitespaces(elem) == e) return "1";
            }
          }
          return "0";
        } },
  {"~:", [](const std::string& a, const std::string& b) -> std::string { 
          std::string res_vec = "";
          for (const auto& elem : util::Split(b.substr(1, b.length()-1), ";")) {
            auto res = fuzzy::fuzzy(DeleteWhitespaces(elem), a);
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
};

ExpressionParser::ExpressionParser() {
  opts_simple_ = {">", "<", "=", "~", "+"};
}

ExpressionParser::ExpressionParser(std::map<std::string, std::string> substitute) {
  opts_simple_ = {">", "<", "=", "~", "+"};
  substitue_ = substitute;
}

std::string ExpressionParser::evaluate(std::string input) {
  std::cout << input << std::endl;
  auto [pos, opt] = LastOpt(input); 

  // single entry
  if (pos == -1) {
    return DeleteWhitespaces(input);
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
  std::string next = DeleteWhitespaces(input.substr(pos+opt.length(), input.length()-(pos + opt.length()-1)));
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

int ExpressionParser::Success(std::string input) {
  if (input == "") return true;

  DeleteNonsense(input);
  
  // Check if for unnecessary surounding brackets
  while (input.front() == '(' && MatchingBracket(input) == input.length()-1) {
      //&& NumFind(input, " | ") < 1 && MatchingBracket(input, " & ") < 1) {
    input.erase(0, 1);
    input.pop_back();
  }

  // Check if already elementary form
  // TODO (fux): check for only one operator
  if (input.find("(") == std::string::npos) {
    auto vec = MatchingBracketAtOperator(input);
    return Calc(vec[0], vec[1], vec[2]);
  }

  // Recursively create elementary form
  auto vec = GetBetween(input);
  if (vec[0].find("(") == std::string::npos && vec[2].find("(") 
      == std::string::npos)
    return Evaluate(vec[0], vec[1], vec[2]);
  if (vec[0].find("(") == std::string::npos)
    return Evaluate(vec[0], vec[1], evaluate(vec[2])); 
  else if (vec[2].find("(") == std::string::npos)
    return Evaluate(evaluate(vec[0]), vec[1], vec[2]);
  else 
    return Evaluate(evaluate(vec[0]), vec[1], evaluate(vec[2]));
}

bool ExpressionParser::Calc(std::string str1, std::string opt, std::string str2) { 
	std::cout << "ExpressionParser::Calc: " << str1 << ", " << opt << ", " << str2  << std::endl;
  //Check if or ("|") is included in second string.
  if (str2.find("|") != std::string::npos) {
    return Calc(str1, opt, str2.substr(0, str2.find("|"))) ||
      Calc(str1, opt, str2.substr(str2.find("|")+1));
  }
  //Check if or ("&") is included in second string.
  if (str2.find("&") != std::string::npos) {
    return Calc(str1, opt, str2.substr(0, str2.find("&"))) &&
      Calc(str1, opt, str2.substr(str2.find("&")+1));
  }

  //Check if term shall be negated
  bool negation = true;
  if (str1.front() == '!') {
    negation = false;
    str1.erase(0,1);
  }

  // Substitut first or second literal
  if (str1.front() == '\'')
    str1.erase(0,1);
  else if (substitue_.count(str1) > 0)
    str1 = substitue_[str1];
  if (str2.front() == '\'') {
    str2.erase(0,1);
		str2.pop_back();
	}
  else if (substitue_.count(str2) > 0) {
    str2 = substitue_[str2];
	}

	std::cout << "ExpressionParser: After subsituting: " << str1 << ", " << opt << ", " << str2  << std::endl;

  //Calculate value.
  bool result = true;
  if (opt == "=") 
		result = str1 == str2;
  else if (opt == "~" && str1.find(";"))  {
    auto vec = Split(str1, ";");
		std::cout << "LP: Checking against list: " << str2 << std::endl;
    result = false;
		for (const auto& it : vec) {
			std::cout << "LP: - " << it << std::endl;
      //result = result | fuzzy::cmp(it, str2);
    }
  }
  // else if (opt == "~")
  //   result = fuzzy::fuzzy_cmp(str2, str1) <= 0.2;
  else if (opt == ">") 
    result = std::stoi(str1) > std::stoi(str2);
  else if (opt == "<") 
    result = std::stoi(str1) < std::stoi(str2);
  else if(opt == ":") {
    auto vec = Split(str1, ";");
		std::cout << "LP: Checking against list: " << str2 << std::endl;
		for (const auto& it : vec) 
			std::cout << "LP: - " << it << std::endl;
    // result = std::find(vec.begin(), vec.end(), str2) != vec.end();
  }
  else
    std::cout << "Wrong operand (=, ~, >, <)!" << std::endl;

  if (negation == true) return result;
  return !result;
}

bool ExpressionParser::Evaluate(std::string str1, std::string opt, std::string str2) {
  auto vec1 = MatchingBracketAtOperator(str1);
  auto vec2 = MatchingBracketAtOperator(str2);
  return Evaluate(Calc(vec1[0], vec1[1], vec1[2]), opt, 
      Calc(vec2[0], vec2[1], vec2[2])); 
}

bool ExpressionParser::Evaluate(bool x, std::string opt, std::string str2) {
  auto vec = MatchingBracketAtOperator(str2);
  return Evaluate(x, opt, Calc(vec[0], vec[1], vec[2])); 
}

bool ExpressionParser::Evaluate(std::string str1, std::string opt, bool x) {
  auto vec = MatchingBracketAtOperator(str1);
  return Evaluate(Calc(vec[0], vec[1], vec[2]), opt, x); 
}

bool ExpressionParser::Evaluate(bool x, std::string opt, bool y) { 
  if (opt == "|") return x || y;
  if (opt == "&") return x && y;
  std::cout << "Wrong operand! (|, &)!" << std::endl;
  return true; 
}

std::vector<std::string> ExpressionParser::GetBetween(std::string input) {
  size_t pos = input.find("(");
  if (pos == std::string::npos) 
    return MatchingBracketAtOperator(input);
  else {
    pos = MatchingBracket(input);
    std::string str = input.substr(pos+3);
    return {input.substr(1, pos-1), input.substr(pos + 2, 1), str};
  }
}

size_t ExpressionParser::MatchingBracket(std::string str) {
  size_t num = 0;
  for (size_t x=0; x<str.length(); x++) {
    if (str[x] == '(')
      num++;
    else if (str[x] == ')' && num == 1)
      return x;
    else if (str[x] == ')')
      num--;
  }
  std::cout << "\nFailed!!\n";
  return 0;
}

std::vector<std::string> ExpressionParser::MatchingBracketAtOperator(std::string 
    str) {
  std::string opt = "";
  size_t pos = 0;
  for (pos=0; pos<str.length(); pos++) {
    // if (std::find(opts_.begin(), opts_.end(), str[pos]) != opts_.end()) {
    //   opt = str[pos];
    //   break;
    // }
  }
  return {str.substr(0, pos), opt, str.substr(pos+1)};
}

size_t ExpressionParser::NumFind(std::string str, std::string m) {
  size_t num=0;
  size_t pos = str.find(m);
  while (pos != std::string::npos) {
    num++;
    pos = str.find(m, pos+m.length());
  }
  return num;
}

std::vector<std::string> ExpressionParser::Split(std::string str, std::string 
    delimiter) {
  std::vector<std::string> strings;
  size_t pos=0;
  while ((pos = str.find(delimiter)) != std::string::npos) {
    if(pos!=0)
      strings.push_back(str.substr(0, pos));
    str.erase(0, pos + delimiter.length());
  }

  //Push ending of the string and return
  strings.push_back(str);
  return strings;
}

void ExpressionParser::DeleteNonsense(std::string& str) {
  for(;;) {
    if 
      (str.front() == ' ') str.erase(0, 1);
    else if 
      (str.back() == ' ') str.pop_back();
    else 
      break;
  }
}

std::string ExpressionParser::DeleteWhitespaces(const std::string& str) {
  std::string editable_str = str; 
  DeleteNonsense(editable_str);
  return editable_str;
}
