#include "game/utils/defines.h"
#include "shared/utils/utils.h"
#define CATCH_CONFIG_RUNNER
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <stdlib.h>
#include <time.h>
#include <spdlog/spdlog.h>


int main( int argc, char* argv[] ) {
  /* initialize random seed: */
  srand (time(NULL));

  util::SetUpLogger(txtad::FILES_PATH, txtad::TEST_LOGGER, spdlog::level::info);

  util::LoggerContext scope(txtad::TEST_LOGGER);
  int result = Catch::Session().run( argc, argv );

  return result;
}
