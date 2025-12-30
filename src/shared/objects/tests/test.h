#ifndef SRC_SHARED_OBJECTS_TESTS_TEST_H_
#define SRC_SHARED_OBJECTS_TESTS_TEST_H_

#include <vector>
#include <nlohmann/json.hpp>
#include <string>

class Game;

class Test {
  public: 
    typedef std::pair<bool, std::string> _t_test_result;

    Test(const nlohmann::json& test);

    // Methods
    _t_test_result Run(Game& game, const std::string& user_id, std::string& res) const;
  
  private: 
    const std::string _cmd;
    const std::string _result;
    const std::vector<std::string> _checks;

};

#endif
