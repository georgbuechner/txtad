#include "game/game/game.h"
#include "game/utils/defines.h"
#include "shared/utils/defines.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/utils.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

class TestGameWrapper {
  private:
    const std::string _path;

  public: 
    TestGameWrapper(const std::string& name, const nlohmann::json& settings, 
          const std::map<std::string, std::vector<nlohmann::json>>& contexts,
          const std::map<std::string, nlohmann::json>& texts) 
        : _path(txtad::GAMES_PATH + "/" + name) {
      const std::string game_files_path = _path + "/" + txtad::GAME_FILES;
      fs::create_directory(_path);

      // Write settings to disc
      util::WriteJsonToDisc(_path + "/" + txtad::GAME_SETTINGS, settings);

      fs::create_directory(game_files_path);
      // Write texts to disc 
      for (const auto& [path, json] : texts) {
        const std::string file_path = game_files_path + ((path !="") ? "/" + path : "");
        if (!fs::exists(file_path)) {
          fs::create_directories(file_path);
        }
        util::WriteJsonToDisc(file_path + txtad::TEXT_EXTENSION, json);
      }

      // Write contexts to disc
      for (const auto& [path, jsons] : contexts) {
        const std::string file_path = game_files_path + ((path !="") ? "/" + path : "");
        if (!fs::exists(file_path)) {
          fs::create_directories(file_path);
        }
        for (const auto& json : jsons)
          util::WriteJsonToDisc(file_path + "/" + json.at("id").get<std::string>() + txtad::CONTEXT_EXTENSION,
              json);
      }
    }
    ~TestGameWrapper() {
      fs::remove_all(_path);
    }
};

TEST_CASE("Test Creating Game", "[game]") {
  const nlohmann::json settings = {
    {"initial_events", "#print texts.START"},
    {"initial_contexts", {"rooms/room_1"}}
  };

  const nlohmann::json ctx_general = {
    {"id", "general"},
    {"name", "General"},
    {"description", "Some gegnaral handlers"},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "#ctx replace *rooms -> (.*)"}, {"arguments", 
        "#> You've entered {ctx.*rooms->description}"}, {"permeable", true}}
    }},
  };

  const nlohmann::json ctx_room_1 = {
    {"id", "room_1"},
    {"name", "Room 1"},
    {"description", "Test room no. 1"},
    {"attributes", {{"gravity", "10"}, {"darkness", "99"}}},
    {"listeners", {
      {{"id", "L2"}, {"re_event", "go right"}, {"ctx", "rooms/room_2"}, {"arguments", "#ctx replace *rooms -> <ctx>"}, 
        {"permeable", true}, {"use_ctx_regex", UseCtx::NO}}
    }},
  };

  const nlohmann::json ctx_room_2 = {
    {"id", "room_2"},
    {"name", "Room 2"},
    {"description", "Test room no. 1"},
    {"attributes", {{"gravity", "99"}, {"darkness", "10"}}},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "go (.*)"}, {"ctx", "rooms/room_1"}, {"arguments", "#ctx replace *rooms -> <ctx>"}, 
        {"permeable", true}, {"use_ctx_regex", UseCtx::Name}}
    }},
  };

  const nlohmann::json txt_text_1 = {
    {"txt", "Hello World"},
    {"permanent_events", "#print texts.random"},
    {"one_time_events", "#sa hp += 10"},
  };

  const std::string GAME_NAME = "test_game";
  const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
  TestGameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}, {"rooms", {ctx_room_1, 
      ctx_room_2}}}, {{"texts/start", txt_text_1}});

  Game game(GAME_PATH, GAME_NAME);

  // Test game created successfully
  REQUIRE(game.name() == GAME_NAME);
  REQUIRE(game.path() == GAME_PATH); 
  // Test settings created successfully
  REQUIRE(game.settings().initial_events() == settings["initial_events"]);
  REQUIRE(game.settings().initial_ctx_ids().size() == settings["initial_contexts"].size());
  REQUIRE(game.settings().initial_ctx_ids().front() == "rooms/room_1");
  // Test contexts created successfully
  REQUIRE(game.contexts().count("rooms/room_1") > 0); 
  auto ctx = game.contexts().at("rooms/room_1");
  REQUIRE(ctx->id() == "rooms/" + ctx_room_1["id"].get<std::string>()); 
  REQUIRE(ctx->name() == ctx_room_1["name"]); 
  // Tests attributes
  REQUIRE(ctx->GetAttribute("gravity") == ctx_room_1["attributes"]["gravity"]); 
  REQUIRE(ctx->GetAttribute("darkness") == ctx_room_1["attributes"]["darkness"]); 

  // Create user: 
  const std::string USER_ID = "0x1234";
  game.HandleEvent(USER_ID, "");

  // Test events
  ExpressionParser parser;
  util::Logger()->info("GO LEFT");
  REQUIRE(ctx->TakeEvent("go left", parser) == false);
  util::Logger()->info("GO RIGHT");
  REQUIRE(ctx->TakeEvent("go right", parser) == true);
  auto ctx_2 = game.contexts().at("rooms/room_2");
  util::Logger()->info("GO LEFT");
  REQUIRE(ctx_2->TakeEvent("go left", parser) == false);
  util::Logger()->info("GO ROOM 1");
  REQUIRE(ctx_2->TakeEvent("go Room 1", parser) == true);
  util::Logger()->info("done");

  // Test texts 
  REQUIRE(game.texts().count("texts/start") > 0);
  std::string event_queue;
  REQUIRE(game.texts().at("texts/start")->print(event_queue) == txt_text_1["txt"].get<std::string>());
  REQUIRE(event_queue == txt_text_1["permanent_events"].get<std::string>() + ";" 
      + txt_text_1["one_time_events"].get<std::string>());
  event_queue = "";
  // After second print, event_queue does not contain onetime events.
  REQUIRE(game.texts().at("texts/start")->print(event_queue) == txt_text_1["txt"].get<std::string>());
  REQUIRE(event_queue == txt_text_1["permanent_events"].get<std::string>());
}
