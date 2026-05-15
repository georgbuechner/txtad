#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include "builder/game/builder_game.h"
#include "game/utils/defines.h"
#include "shared/utils/defines.h"
#include "shared/utils/test_helpers.h"

TEST_CASE("Test Creating Listener", "[builder]") {
  const std::string LISTENER_ID = "L2";
  const std::string CTX_ID = "room_1";
  const std::string ROOM_ID = "rooms/" + CTX_ID;
  const std::string ROOM_2_ID = "rooms/room_2";

  const nlohmann::json settings = {
    {"initial_events", ""},
    {"initial_contexts", {"rooms/room_1"}}
  };

  const nlohmann::json ctx_room_1 = {
    {"id", CTX_ID},
    {"name", "Room 1"},
    {"description", "Test room no. 1"},
    {"attributes", {{"gravity", "10"}, {"darkness", "99"}}},
    {"listeners", nlohmann::json::array() },
  };
  const nlohmann::json ctx_room_2 = {
    {"id", "room_2"},
    {"name", "Room 2"},
    {"description", "Test room no. 1"},
    {"attributes", {{"gravity", "99"}, {"darkness", "10"}}},
    {"listeners", nlohmann::json::array() },
  };


  const nlohmann::json j_listener = {
    {"id", LISTENER_ID}, {"re_event", "go right"}, {"ctx", ROOM_2_ID}, 
    {"arguments", "#ctx replace *rooms -> <ctx>"}, {"permeable", true}, 
    {"logic", "{<ctx>.gravity} = 99"}, {"permeable", true}, 
    {"use_ctx_regex", UseCtx::NO}};

  const std::string GAME_NAME = "test_game";
  const std::string GAME_PATH = txtad::GamesPath() + GAME_NAME;
  test::GameWrapper test_game_wrapper(GAME_NAME, settings, { {"rooms", {ctx_room_1, ctx_room_2}} }, {});

  BuilderGame builder(GAME_PATH, GAME_NAME);

  builder.CreateListenerInPlace(LISTENER_ID, j_listener, ROOM_ID, true);

  REQUIRE(builder.modified().front() == "Added listener " + LISTENER_ID + " for ctx: " + ROOM_ID);
  
  // Create user: 
  const std::string USER_ID = "0x1234";
  builder.HandleEvent(USER_ID, "");

  // Test new listener fires
  auto room = builder.contexts().at(ROOM_ID);
  REQUIRE(room->TakeEvent("go right", builder.parser()) == true);
  REQUIRE(room->listeners().at(LISTENER_ID)->ctx_id() == ROOM_2_ID);
  REQUIRE(room->listeners().at(LISTENER_ID)->arguments() == "#ctx replace *rooms -> " + ROOM_2_ID);
  REQUIRE(room->listeners().at(LISTENER_ID)->logic() == "{" + ROOM_2_ID + ".gravity} = 99");
}
