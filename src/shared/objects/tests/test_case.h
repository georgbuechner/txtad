#ifndef SRC_SHARED_OBJECTS_TESTS_TEST_CASE_H_
#define SRC_SHARED_OBJECTS_TESTS_TEST_CASE_H_

#include <nlohmann/json.hpp>
#include <vector>
#include "builder/game/builder_game.h"
#include "shared/objects/tests/test.h"

class game;

class TestCase {
  public: 
    TestCase();
    TestCase(const nlohmann::json& json);

    // getter
    const std::string& desc() const;
    const std::vector<Test>& tests() const;

    // methods 
    
    /** 
     * Initial connection (f.e. "#new_connection" not automatically run!
     */
    std::string Run(std::shared_ptr<BuilderGame> game) const;

  private: 
    const std::string _desc;
    std::vector<Test> _tests;
};

#endif
