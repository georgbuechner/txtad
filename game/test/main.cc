#include "utils/defines.h"
#include "utils/utils.h"
#include <iostream>
#define CATCH_CONFIG_RUNNER
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <stdlib.h>
#include <time.h>
#include <spdlog/spdlog.h>


int main( int argc, char* argv[] ) {
  /* initialize random seed: */
  srand (time(NULL));

  util::SetUpLogger(txtad::LOGGER, spdlog::level::info);
  // util::SetUpLogger(txtad::LOGGER, spdlog::level::debug);

  util::LoggerContext scope(txtad::LOGGER);
  int result = Catch::Session().run( argc, argv );

  return result;
}
