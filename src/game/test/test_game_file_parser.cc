#include "builder/game/builder_game.h"
#include "builder/utils/defines.h"
#include "game/utils/defines.h"
#include "shared/utils/parser/game_file_parser.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Test get paths", "[gf_parser]") {
  SECTION ("Get requesting all paths") {
    auto res = parser::GetPaths(txtad::GAMES_PATH + "original");
    REQUIRE(res.at("items") == builder::FileType::DIR);
    REQUIRE(res.at("catch") == builder::FileType::CTX);
    REQUIRE(res.at("general") == builder::FileType::CTX);
    REQUIRE(res.at("start") == builder::FileType::TXT);
    REQUIRE(res.at("items/heiltrank") == builder::FileType::CTX);
  }
  SECTION ("Get requesting only root") {
    auto res = parser::GetPaths(txtad::GAMES_PATH + "original", "");
    REQUIRE(res.at("items") == builder::FileType::DIR);
    REQUIRE(res.at("catch") == builder::FileType::CTX);
    REQUIRE(res.at("general") == builder::FileType::CTX);
    REQUIRE(res.at("start") == builder::FileType::TXT);
    REQUIRE(!res.contains("items/heiltrank"));
  }
  SECTION ("Get requesting only deeper path") {
    auto res = parser::GetPaths(txtad::GAMES_PATH + "original", "items");
    REQUIRE(res.at("heiltrank") == builder::FileType::CTX);
    REQUIRE(!res.contains("items"));
    REQUIRE(!res.contains("catch"));
    REQUIRE(!res.contains("general"));
    REQUIRE(!res.contains("start"));
  }
  
  BuilderGame game(txtad::GAMES_PATH + "original", "original");
  SECTION ("Get requesting all paths") {
    auto res = game.GetPaths();
    REQUIRE(res.at("items") == builder::FileType::DIR);
    REQUIRE(res.at("catch") == builder::FileType::CTX);
    REQUIRE(res.at("general") == builder::FileType::CTX);
    REQUIRE(res.at("start") == builder::FileType::TXT);
    REQUIRE(res.at("items/heiltrank") == builder::FileType::CTX);
  }
  SECTION ("Get requesting only root") {
    auto res = game.GetPaths("");
    REQUIRE(res.at("items") == builder::FileType::DIR);
    REQUIRE(res.at("catch") == builder::FileType::CTX);
    REQUIRE(res.at("general") == builder::FileType::CTX);
    REQUIRE(res.at("start") == builder::FileType::TXT);
    REQUIRE(!res.contains("items/heiltrank"));
  }
  SECTION ("Get requesting only deeper path") {
    auto res = game.GetPaths("items");
    REQUIRE(res.at("heiltrank") == builder::FileType::CTX);
    REQUIRE(!res.contains("items"));
    REQUIRE(!res.contains("catch"));
    REQUIRE(!res.contains("general"));
    REQUIRE(!res.contains("start"));
  }

}
