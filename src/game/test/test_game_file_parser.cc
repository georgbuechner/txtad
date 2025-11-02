#include "builder/utils/defines.h"
#include "game/utils/defines.h"
#include "shared/utils/parser/game_file_parser.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Test get paths", "[gf_parser]") {
  SECTION ("Get requesting all paths") {
    auto res = parser::GetPaths(txtad::GAMES_PATH + "original");
    for (const auto& it : res) 
    REQUIRE(res["items"] == builder::FileType::DIR);
    REQUIRE(res["catch"] == builder::FileType::CTX);
    REQUIRE(res["general"] == builder::FileType::CTX);
    REQUIRE(res["start"] == builder::FileType::TXT);
    REQUIRE(res["items/heiltrank"] == builder::FileType::CTX);
  }
  SECTION ("Get requesting only root") {
    auto res = parser::GetPaths(txtad::GAMES_PATH + "original", "");
    REQUIRE(res["items"] == builder::FileType::DIR);
    REQUIRE(res["catch"] == builder::FileType::CTX);
    REQUIRE(res["general"] == builder::FileType::CTX);
    REQUIRE(res["start"] == builder::FileType::TXT);
    REQUIRE(!res.contains("heiltrank"));
  }
  SECTION ("Get requesting only deeper path") {
    auto res = parser::GetPaths(txtad::GAMES_PATH + "original", "items");
    REQUIRE(res["heiltrank"] == builder::FileType::CTX);
    REQUIRE(!res.contains("items"));
    REQUIRE(!res.contains("catch"));
    REQUIRE(!res.contains("general"));
    REQUIRE(!res.contains("start"));
  }

}
