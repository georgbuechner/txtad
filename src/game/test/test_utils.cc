#include "game/utils/defines.h"
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
