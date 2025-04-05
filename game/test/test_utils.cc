#include "utils/defines.h"
#include "utils/utils.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <vector>

TEST_CASE("Test getting random number", "[util]") {
  for (int i=0; i<1000; i++) {
    int ran = util::Ran(10);
    REQUIRE((ran <= 10 && ran >= 0));
  }

  for (int i=0; i<1000; i++) {
    int ran = util::Ran(10, 2);
    REQUIRE((ran <= 10 && ran >= 2));
  }

  for (int i=0; i<1000; i++) {
    int ran = util::Ran(1);
    REQUIRE((ran <= 1 && ran >= 0));
  }

  for (int i=0; i<1000; i++) {
    int ran = util::Ran(0);
    REQUIRE(ran == 0);
  }
}
