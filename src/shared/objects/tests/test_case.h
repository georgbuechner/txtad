#ifndef SRC_SHARED_OBJECTS_TESTS_TEST_CASE_H_
#define SRC_SHARED_OBJECTS_TESTS_TEST_CASE_H_

#include <nlohmann/json.hpp>
#include <vector>
#include "shared/objects/tests/test.h"

class game;

class TestCase {
  public: 
    TestCase(const nlohmann::json json);

    // methods 

    /** 
     * Initial connection (f.e. "#new_connection" not automatically run!
     */
    std::string Run(std::shared_ptr<Game> game) const;

  private: 
    const std::string _desc;
    std::vector<Test> _tests;
};

#endif
