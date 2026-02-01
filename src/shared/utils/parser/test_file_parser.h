#ifndef SHARED_UTILS_PARSER_TEST_FILE_PARSER_H_
#define SHARED_UTILS_PARSER_TEST_FILE_PARSER_H_

#include <string>
#include <vector>

class TestCase;

namespace test_parser {
  std::vector<TestCase> LoadTestCases(const std::string& game_id);
}

#endif 
