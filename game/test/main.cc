#include <iostream>
#define CATCH_CONFIG_RUNNER
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <stdlib.h>
#include <time.h>


int main( int argc, char* argv[] ) {
  /* initialize random seed: */
  srand (time(NULL));

  int result = Catch::Session().run( argc, argv );

  return result;
}
