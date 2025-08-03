#include "game/utils/defines.h"
#include "shared/utils/fuzzy_search/fuzzy.h"
#include "shared/utils/utils.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Test LoggerContext", "[utils]") {
  const std::string INITIAL_LOGGER = "hello";
  const std::string NEW_LOGGER = "world";
  util::SetUpLogger(txtad::FILES_PATH, INITIAL_LOGGER, spdlog::level::info);
  util::SetUpLogger(txtad::FILES_PATH, NEW_LOGGER, spdlog::level::info);

  // Test simple scopes
  REQUIRE(util::LOGGER == txtad::TEST_LOGGER);
  {
    util::LoggerContext logger_scope(INITIAL_LOGGER);
    REQUIRE(util::LOGGER == INITIAL_LOGGER);
  }
  REQUIRE(util::LOGGER == txtad::TEST_LOGGER);
  {
    util::LoggerContext logger_scope(NEW_LOGGER);
    REQUIRE(util::LOGGER == NEW_LOGGER);
  }
  REQUIRE(util::LOGGER == txtad::TEST_LOGGER);

  // Test nested scopes
  {
    util::LoggerContext logger_scope(INITIAL_LOGGER);
    REQUIRE(util::LOGGER == INITIAL_LOGGER);
    {
      util::LoggerContext logger_scope(NEW_LOGGER);
      REQUIRE(util::LOGGER == NEW_LOGGER);
    }
    REQUIRE(util::LOGGER == INITIAL_LOGGER);
  }
  REQUIRE(util::LOGGER == txtad::TEST_LOGGER);
}

TEST_CASE("Test fuzzy search", "[utils]") {
  const std::string A = "elefant";
  const std::string B = "elephant";

  // In general fuzzy_cmp(A, B) != fuzzy_cmp(B, A) 
  REQUIRE(fuzzy::fuzzy_cmp(A, B) <= 0.25);
  REQUIRE(fuzzy::fuzzy(A, B) == fuzzy::FuzzyMatch::FUZZY);
  REQUIRE(fuzzy::fuzzy_cmp(B, A) <= 0.3);
  REQUIRE(fuzzy::fuzzy(B, A) == fuzzy::FuzzyMatch::FUZZY);
}

TEST_CASE("Test GetUserId" "[util]") {
  const std::string USER_ID = "0x7f4fd40074b0";
  const std::string INP_PART = "Fuck you!"; 

  std::string full_inp = USER_ID + INP_PART;

  std::string user_id = util::GetUserId(full_inp).value_or("");
  REQUIRE(user_id == USER_ID);
  REQUIRE(full_inp == INP_PART);
}
