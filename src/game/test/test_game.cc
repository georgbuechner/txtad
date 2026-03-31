#include "builder/game/builder_game.h"
#include "game/game/game.h"
#include "game/utils/defines.h"
#include "shared/utils/defines.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/parser/test_file_parser.h"
#include "shared/utils/test_helpers.h"
#include "shared/utils/utils.h"
#include "shared/objects/tests/test_case.h"
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

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
      {{"id", "L2"}, {"re_event", "go right"}, {"ctx", "rooms/room_2"}, 
        {"arguments", "#ctx replace *rooms -> <ctx>"}, {"permeable", true}, 
        {"logic", "{<ctx>.gravity} = 99"}, {"permeable", true}, 
        {"use_ctx_regex", UseCtx::NO}}
    }},
  };

  const nlohmann::json ctx_room_2 = {
    {"id", "room_2"},
    {"name", "Room 2"},
    {"description", "Test room no. 1"},
    {"attributes", {{"gravity", "99"}, {"darkness", "10"}}},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "go (.*)"}, {"ctx", "rooms/room_1"}, {"arguments", "#ctx replace *rooms -> <ctx>"}, 
        {"permeable", true}, {"use_ctx_regex", UseCtx::NAME}}
    }},
  };

  const nlohmann::json txt_text_1 = {
    {"txt", "Hello World"},
    {"permanent_events", "#print texts.random"},
    {"one_time_events", "#sa hp += 10"},
  };

  const std::string GAME_NAME = "test_game";
  const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
  test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}, {"rooms", {ctx_room_1, 
      ctx_room_2}}}, {{"texts/start", txt_text_1}});

  Game game(GAME_PATH, GAME_NAME);

  // Test game created successfully
  REQUIRE(game.name() == GAME_NAME);
  REQUIRE(game.path() == GAME_PATH); 
  // Test settings created successfully
  REQUIRE(game.settings().initial_events() == settings["initial_events"].get<std::string>());
  REQUIRE(game.settings().initial_ctx_ids().size() == settings["initial_contexts"].size());
  REQUIRE(game.settings().initial_ctx_ids().front() == "rooms/room_1");
  // Test contexts created successfully
  REQUIRE(game.contexts().count("rooms/room_1") > 0); 
  auto ctx = game.contexts().at("rooms/room_1");
  REQUIRE(ctx->id() == "rooms/" + ctx_room_1["id"].get<std::string>()); 
  REQUIRE(ctx->name() == ctx_room_1["name"].get<std::string>()); 
  // Tests attributes
  REQUIRE(ctx->GetAttribute("gravity") == ctx_room_1["attributes"]["gravity"].get<std::string>()); 
  REQUIRE(ctx->GetAttribute("darkness") == ctx_room_1["attributes"]["darkness"].get<std::string>()); 

  // Create user: 
  const std::string USER_ID = "0x1234";
  game.HandleEvent(USER_ID, "");

  // Test events
  util::Logger()->info("GO LEFT");
  REQUIRE(ctx->TakeEvent("go left", game.parser()) == false);
  util::Logger()->info("GO RIGHT");
  REQUIRE(ctx->TakeEvent("go right", game.parser()) == true);
  auto ctx_2 = game.contexts().at("rooms/room_2");
  util::Logger()->info("GO LEFT");
  REQUIRE(ctx_2->TakeEvent("go left", game.parser()) == false);
  util::Logger()->info("GO ROOM 1");
  REQUIRE(ctx_2->TakeEvent("go Room 1", game.parser()) == true);
  util::Logger()->info("done");

  // Test texts 
  REQUIRE(game.texts().count("texts/start") > 0);
  std::string event_queue;
  REQUIRE(util::Join(game.texts().at("texts/start")->print(event_queue, game.parser()), ", ") 
      == txt_text_1["txt"].get<std::string>());
  REQUIRE(event_queue == txt_text_1["permanent_events"].get<std::string>() + ";" 
      + txt_text_1["one_time_events"].get<std::string>());
  event_queue = "";
  // After second print, event_queue does not contain onetime events.
  REQUIRE(util::Join(game.texts().at("texts/start")->print(event_queue, game.parser()), ", ") 
      == txt_text_1["txt"].get<std::string>());
  REQUIRE(event_queue == txt_text_1["permanent_events"].get<std::string>());
}

TEST_CASE("Test two games and multiple users (shared)", "[game]") {
  const nlohmann::json settings = {
    {"initial_events", ""},
    {"initial_contexts", {"general"}}
  };

  const nlohmann::json ctx_general = {
    {"id", "general"},
    {"name", "General"},
    {"description", "Some general handlers"},
    {"attributes", {{"counter", "0"}}},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "increase-counter"}, {"arguments", 
        "#sa general.counter++"}, {"permeable", true}}
    }},
  };

  SECTION ("Test two games") {
    const std::string GAME_1_NAME = "test_game";
    const std::string GAME_1_PATH = txtad::GAMES_PATH + GAME_1_NAME;
    test::GameWrapper test_game_wrapper_1(GAME_1_NAME, settings, {{"", {ctx_general}}}, {});
    Game game_1(GAME_1_PATH, GAME_1_NAME);

    const std::string GAME_2_NAME = "test_game_2";
    const std::string GAME_2_PATH = txtad::GAMES_PATH + GAME_2_NAME;
    test::GameWrapper test_game_wrapper_2(GAME_2_NAME, settings, {{"", {ctx_general}}}, {});
    Game game_2(GAME_2_PATH, GAME_2_NAME);
    
    // Create user(s): 
    const std::string USER_ID = "0x1234"; // they can both have the same ID (non realistic scenario)
    game_1.HandleEvent(USER_ID, "");
    game_2.HandleEvent(USER_ID, "");

    // Game one increases general.counter once (= 1)
    game_1.HandleEvent(USER_ID, "increase-counter");
    REQUIRE(game_1.contexts().at("general")->GetAttribute("counter").value_or("-1") == "1");

    // Game two increases general.counter twice (= 2)
    game_2.HandleEvent(USER_ID, "increase-counter");
    game_2.HandleEvent(USER_ID, "increase-counter");
    REQUIRE(game_2.contexts().at("general")->GetAttribute("counter").value_or("-1") == "2");
    REQUIRE(game_1.contexts().at("general")->GetAttribute("counter").value_or("-1") == "1");

    // Game one's general.counter stays untouched
    REQUIRE(game_1.contexts().at("general")->GetAttribute("counter").value_or("-1") == "1");
  }

  SECTION ("Test one games and two user") {
    // Create game
    const std::string GAME_NAME = "test_game";
    const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
    test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}}, {});
    Game game(GAME_PATH, GAME_NAME);

    // Create users
    const std::string G1_USER_ID = "0x1234";
    game.HandleEvent(G1_USER_ID, "");
    const std::string G2_USER_ID = "0x1235";
    game.HandleEvent(G2_USER_ID, "");

    game.HandleEvent(G1_USER_ID, "increase-counter");
    REQUIRE(game.contexts().at("general")->GetAttribute("counter").value_or("-1") == "1");
    game.HandleEvent(G2_USER_ID, "increase-counter");
    REQUIRE(game.contexts().at("general")->GetAttribute("counter").value_or("-1") == "2");
  }
}

TEST_CASE("Test two users and non-shared contexts", "[game]") {
  const nlohmann::json settings = {
    {"initial_events", ""},
    {"initial_contexts", {"general"}}
  };

  const nlohmann::json ctx_general = {
    {"id", "general"},
    {"name", "General"},
    {"description", "Some general handlers"},
    {"shared", false},
    {"attributes", {{"counter", "0"}}},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "increase-counter"}, {"arguments", 
        "#sa general.counter++"}, {"permeable", true}}
    }},
  };

  SECTION ("Test one games and two user") {
    // Create game
    const std::string GAME_NAME = "test_game";
    const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
    test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}}, {});
    Game game(GAME_PATH, GAME_NAME);

    // Create users
    const std::string USER_ID_1 = "0x1234";
    game.HandleEvent(USER_ID_1, "");
    const std::string USER_ID_2 = "0x1235";
    game.HandleEvent(USER_ID_2, "");

    game.HandleEvent(USER_ID_1, "increase-counter");
    REQUIRE(game.cur_user()->contexts().at("general")->GetAttribute("counter").value_or("-1") == "1");
    game.HandleEvent(USER_ID_2, "increase-counter");
    REQUIRE(game.cur_user()->contexts().at("general")->GetAttribute("counter").value_or("-1") == "1");
  }
}

TEST_CASE("Test Game handlers/mechanics", "[game]") {
  const nlohmann::json settings = {
    {"initial_events", ""},
    {"initial_contexts", {"general"}}
  };

  const nlohmann::json ctx_general = {
    {"id", "general"},
    {"name", "General"},
    {"description", "Some general handlers"},
    {"shared", false},
    {"attributes", {{"counter", "0"}, {"_hidden", "true"}}},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "increase-counter"}, {"arguments", 
        "#sa general.counter++"}, {"permeable", true}}
    }},
  };

  const nlohmann::json ctx_room_1 = {
    {"id", "room_1"},
    {"name", "Room 1"},
    {"description", "Test room no. 1"},
    {"attributes", {{"gravity", "10"}, {"darkness", "99"}}},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "go to (.*)"}, {"ctx", "rooms/room_2"}, 
        {"arguments", "#ctx replace *rooms -> <ctx>"}, {"use_ctx_regex", UseCtx::NAME},
        {"permeable", false}},
      {{"id", "L2"}, {"re_event", "go to (.*)"}, {"ctx", "rooms/room_3"}, 
        {"arguments", "#ctx replace *rooms -> <ctx>"}, {"use_ctx_regex", UseCtx::NAME},
        {"permeable", false}},
      {{"id", "L3"}, {"re_event", "pick up (.*)"}, {"ctx", "items/item_1"}, 
        {"arguments", "#ctx add <ctx>"}, {"use_ctx_regex", UseCtx::NAME},
        {"permeable", false}}
    }},

  };

  const nlohmann::json ctx_room_2 = {
    {"id", "room_2"},
    {"name", "Room 2"},
    {"description", "Test room no. 2"},
    {"attributes", {{"gravity", "99"}, {"darkness", "9"}}}
  };

  const nlohmann::json ctx_room_3 = {
    {"id", "room_3"},
    {"name", "Room 3"},
    {"description", "Test room no. 3"},
    {"attributes", {{"gravity", "99"}, {"darkness", "9"}}}
  };

  const nlohmann::json ctx_item_1 = {
    {"id", "item_1"},
    {"name", "Item 1"},
    {"description", "Test item no. 1"},
    {"attributes", {{"gravity", "99"}, {"darkness", "9"}}}
  };


  const nlohmann::json txt_text_1 = {
    {"txt", "Hello World"},
    {"permanent_events", "#print texts.random"},
    {"one_time_events", ""},
  };

  std::string cout = "";
  Game::set_global_msg_fn([&cout](std::string id, std::string txt) { 
      cout += ((cout != "") ? "\n" : "") + txt; });
  auto get_cout = [&cout]() { 
    std::string str = cout;
    cout = "";
    return str; 
  };


  SECTION ("Test h_add_ctx and h_remove_ctx") {
    // Create game
    const std::string GAME_NAME = "test_game";
    const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
    test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}, {"items", {ctx_item_1}},
        {"rooms", {ctx_room_1, ctx_room_2, ctx_room_3}}}, {});
    Game game(GAME_PATH, GAME_NAME);

    // Create users
    const std::string USER_ID = "0x1234";
    game.HandleEvent(USER_ID, "");

    const std::string ROOM_ID = "rooms/room_1";


    REQUIRE_FALSE(game.cur_user()->context_stack().exists(ROOM_ID));
    game.HandleEvent(USER_ID, fmt::format("#ctx add {}", ROOM_ID));
    REQUIRE(game.cur_user()->context_stack().exists(ROOM_ID));
    game.HandleEvent(USER_ID, fmt::format("#ctx remove {}", ROOM_ID));
    REQUIRE_FALSE(game.cur_user()->context_stack().exists(ROOM_ID));
  }

  SECTION ("Test h_replace_ctx") {
    const nlohmann::json settings = {
      {"initial_events", ""},
      {"initial_contexts", {"general", "rooms/room_1"}}
    };

    // Create game
    const std::string GAME_NAME = "test_game";
    const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
    test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}, {"items", {ctx_item_1}},
        {"rooms", {ctx_room_1, ctx_room_2, ctx_room_3}}}, {});
    Game game(GAME_PATH, GAME_NAME);

    // Create users
    const std::string USER_ID = "0x1234";
    game.HandleEvent(USER_ID, "");

    const std::string ROOM_ID = "rooms/room_1";
    const std::string ROOM_2_ID = "rooms/room_2";

    // test: `#ctx replace <ctx-1-id> -> <ctx-2-id>
    game.HandleEvent(USER_ID, fmt::format("#ctx replace {} -> {}", ROOM_ID, ROOM_2_ID));
    REQUIRE(!game.cur_user()->context_stack().exists(ROOM_ID));
    REQUIRE(game.cur_user()->context_stack().exists(ROOM_2_ID));

    // test: `#ctx replace *<category> -> <ctx-2-id>
    game.HandleEvent(USER_ID, fmt::format("#ctx replace *rooms -> {}", ROOM_ID));
    REQUIRE(game.cur_user()->context_stack().exists(ROOM_ID));
    REQUIRE(!game.cur_user()->context_stack().exists(ROOM_2_ID));
  }

  SECTION ("Test changing ctx name") {
    util::Logger()->info("SECTION \"Test h_print\"");
    const nlohmann::json settings = { 
      {"initial_events", ""}, 
      {"initial_contexts", {"general", "rooms/room_1"}}
    };
    const std::string GAME_NAME = "test_game";
    const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
    test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}, {"items", {ctx_item_1}},
        {"rooms", {ctx_room_1, ctx_room_2, ctx_room_3}}}, {{"texts/start", txt_text_1}});
    Game game(GAME_PATH, GAME_NAME);
    
    // Create users
    const std::string USER_ID = "0x1234";
    game.HandleEvent(USER_ID, "");

    game.HandleEvent(USER_ID, "#ctx name rooms/room_1 = Shady Box");
    game.HandleEvent(USER_ID, "#> {*rooms->name}");
    REQUIRE(get_cout() == "Shady Box");
  }

  SECTION ("Test h_set_attribute") {
    const nlohmann::json settings = { {"initial_events", ""}, {"initial_contexts", {"general"}} };
    const std::string GAME_NAME = "test_game";
    const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
    test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}, {"items", {ctx_item_1}},
        {"rooms", {ctx_room_1, ctx_room_2, ctx_room_3}}}, {});
    Game game(GAME_PATH, GAME_NAME);

    const std::string ATTR_ID = "general.counter";
    const std::string ATTR = "counter";
    
    // Create users
    const std::string USER_ID = "0x1234";
    game.HandleEvent(USER_ID, "");

    // #sa attr=value
    game.HandleEvent(USER_ID, fmt::format("#sa {}=10", ATTR_ID));
    REQUIRE(game.cur_user()->contexts().at("general")->GetAttribute(ATTR).value_or("-1") == "10");
    // #sa attr/=value
    game.HandleEvent(USER_ID, fmt::format("#sa {}/=10", ATTR_ID));
    game.HandleEvent(USER_ID, "#> {" + ATTR_ID + "}");
    REQUIRE(get_cout() == "1");
    // #sa attr*=value
    game.HandleEvent(USER_ID, fmt::format("#sa {}*=10", ATTR_ID));
    game.HandleEvent(USER_ID, "#> {" + ATTR_ID + "}");
    REQUIRE(get_cout() == "10");
    // #sa attr+=value
    game.HandleEvent(USER_ID, fmt::format("#sa {}+=10", ATTR_ID));
    game.HandleEvent(USER_ID, "#> {" + ATTR_ID + "}");
    REQUIRE(get_cout() == "20");
    // #sa attr-=value
    game.HandleEvent(USER_ID, fmt::format("#sa {}-=10", ATTR_ID));
    game.HandleEvent(USER_ID, "#> {" + ATTR_ID + "}");
    REQUIRE(get_cout() == "10");
    // #sa attr--
    game.HandleEvent(USER_ID, fmt::format("#sa {}--", ATTR_ID));
    game.HandleEvent(USER_ID, "#> {" + ATTR_ID + "}");
    REQUIRE(get_cout() == "9");
    // #sa attr++
    game.HandleEvent(USER_ID, fmt::format("#sa {}++", ATTR_ID));
    game.HandleEvent(USER_ID, "#> {" + ATTR_ID + "}");
    REQUIRE(get_cout() == "10");
    // #sa attr+=10*2
    game.HandleEvent(USER_ID, fmt::format("#sa {}+=10*2", ATTR_ID));
    game.HandleEvent(USER_ID, "#> {" + ATTR_ID + "}");
    REQUIRE(get_cout() == "30");
  }

  SECTION ("Test h_list_attributes and h_list_all_attributes") {
    const nlohmann::json settings = { 
      {"initial_events", ""}, 
      {"initial_contexts", {"general"}} 
    };
    const std::string GAME_NAME = "test_game";
    const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
    test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}},{"items", {ctx_item_1}},
        {"rooms", {ctx_room_1, ctx_room_2, ctx_room_3}}}, {});
    Game game(GAME_PATH, GAME_NAME);
    
    // Create users
    const std::string USER_ID = "0x1234";
    game.HandleEvent(USER_ID, "");

    game.HandleEvent(USER_ID, "#lst atts general");
    REQUIRE(get_cout() == "Attributes:\n- counter: 0");
    game.HandleEvent(USER_ID, "#lst* atts general");
    REQUIRE(get_cout() == "Attributes:\n- counter: 0\n- _hidden: true");
  }

  SECTION ("Test h_list_linked_contexts") {
    util::Logger()->info("SECTION \"Test h_print\"");
    const nlohmann::json settings = { 
      {"initial_events", ""}, 
      {"initial_contexts", {"general", "rooms/room_1"}}
    };
    const std::string GAME_NAME = "test_game";
    const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
    test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}, {"items", {ctx_item_1}},
        {"rooms", {ctx_room_1, ctx_room_2, ctx_room_3}}}, {{"texts/start", txt_text_1}});
    Game game(GAME_PATH, GAME_NAME);
    
    // Create users
    const std::string USER_ID = "0x1234";
    game.HandleEvent(USER_ID, "");

    game.HandleEvent(USER_ID, "#> Rooms:;#lst ctxs rooms/room_1->*rooms->name");
    REQUIRE(get_cout() == "Rooms:\n- Room 2\n- Room 3");
    game.HandleEvent(USER_ID, "#> Items:;#lst ctxs rooms/room_1->*items->name");
    REQUIRE(get_cout() == "Items:\n- Item 1");
    game.HandleEvent(USER_ID, "#lst ctxs rooms/room_1->*->name");
    REQUIRE(get_cout() == "- Room 2\n- Room 3\n- Item 1");
    game.HandleEvent(USER_ID, "#> Rooms:;#lst ctxs *rooms->*rooms->name");
    REQUIRE(get_cout() == "Rooms:\n- Room 2\n- Room 3");
  }

  SECTION("Test h_print") {
    util::Logger()->info("SECTION \"Test h_print\"");
    const nlohmann::json settings = { 
      {"initial_events", ""}, 
      {"initial_contexts", {"general", "rooms/room_1"}}
    };
    const std::string GAME_NAME = "test_game";
    const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
    test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}, {"items", {ctx_item_1}},
        {"rooms", {ctx_room_1, ctx_room_2, ctx_room_3}}}, {{"texts/start", txt_text_1}});
    Game game(GAME_PATH, GAME_NAME);
    
    // Create users
    const std::string USER_ID = "0x1234";
    game.HandleEvent(USER_ID, "");


    // Print text 
    game.HandleEvent(USER_ID, "#> {texts/start}");
    REQUIRE(get_cout() == "Hello World");
    
    // Print context name
    game.HandleEvent(USER_ID, "#> {general->name}");
    REQUIRE(get_cout() == "General");
    game.HandleEvent(USER_ID, "#> {rooms/room_2->name}");
    REQUIRE(get_cout() == "Room 2");
    game.HandleEvent(USER_ID, "#> {*rooms->name}");
    REQUIRE(get_cout() == "Room 1");


    // Print context description
    game.HandleEvent(USER_ID, "#> {general->description}");
    REQUIRE(get_cout() == "Some general handlers");
    game.HandleEvent(USER_ID, "#> {rooms/room_2->desc}");
    REQUIRE(get_cout() == "Test room no. 2");
    game.HandleEvent(USER_ID, "#> {*rooms->desc}");
    REQUIRE(get_cout() == "Test room no. 1");

    // Print context attributes
    game.HandleEvent(USER_ID, "#> {general->attributes}");
    REQUIRE(get_cout() == "counter: 0");
    
    // Print contexts attributes (all)
    game.HandleEvent(USER_ID, "#> {general->all_attributes}");
    REQUIRE(get_cout() == "counter: 0, _hidden: true");

    // Print context attribute
    game.HandleEvent(USER_ID, "#> {general.counter}");
    REQUIRE(get_cout() == "0");
    game.HandleEvent(USER_ID, "#> {general._hidden}");
    REQUIRE(get_cout() == "true");
    game.HandleEvent(USER_ID, "#> {rooms/room_1.gravity}");
    REQUIRE(get_cout() == "10");
    game.HandleEvent(USER_ID, "#> {*rooms.darkness}");
    REQUIRE(get_cout() == "99");

    // multible categories: 
    game.HandleEvent(USER_ID, "#ctx add rooms/room_2");

    // Print names of all rooms in stack 
    game.HandleEvent(USER_ID, "#> {*rooms->name}");
    REQUIRE(get_cout() == "Room 1, Room 2");

    // Print desctriptions of all rooms in stack 
    game.HandleEvent(USER_ID, "#> {*rooms->desc}");
    REQUIRE(get_cout() == "Test room no. 1, Test room no. 2");

    // Print contexts in context
    game.HandleEvent(USER_ID, "#> You can go to {*rooms->*rooms->name}");
    REQUIRE(get_cout() == "You can go to Room 2, Room 3");
    game.HandleEvent(USER_ID, "#> You can pick up {*room->*items->name}");
    REQUIRE(get_cout() == "You can pick up Item 1");
    game.HandleEvent(USER_ID, "#> linked contexts: {*room->*->name}");
    REQUIRE(get_cout() == "linked contexts: Room 2, Room 3, Item 1");
    game.HandleEvent(USER_ID, "#> The surrounding darkness {*room->*rooms.darkness}");
    REQUIRE(get_cout() == "The surrounding darkness 9, 9");
  }
}

TEST_CASE("Test exec listeners", "[game]") {
  const nlohmann::json settings = {
    {"initial_events", ""},
    {"initial_contexts", {"general"}}
  };

  const nlohmann::json ctx_general = {
    {"id", "general"},
    {"name", "General"},
    {"description", "Some general handlers"},
    {"shared", false},
    {"attributes", {{"counter", "0"}}},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "increase-counter"}, {"arguments", 
        "#sa general.counter++"}, {"permeable", true}},
      {{"id", "L2"}, {"re_event", "increase-counter"}, {"arguments", 
        "#sa general.counter++"}, {"exec", true}, {"permeable", true}}
    }},
  };

  SECTION ("Test one games and two user") {
    // Create game
    const std::string GAME_NAME = "test_game";
    const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
    test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}}, {});
    Game game(GAME_PATH, GAME_NAME);

    // Create users
    const std::string USER_ID = "0x1234";
    game.HandleEvent(USER_ID, "");

    game.HandleEvent(USER_ID, "increase-counter");
    REQUIRE(game.cur_user()->contexts().at("general")->GetAttribute("counter").value_or("-1") == "2");
  }
}

TEST_CASE("Test game-tests", "[game]") {
  const nlohmann::json settings = {
    {"initial_events", ""},
    {"initial_contexts", {"general", "rooms/room_1"}}
  };

  const nlohmann::json ctx_general = {
    {"id", "general"},
    {"name", "General"},
    {"description", "Some general handlers"},
    {"shared", false},
    {"attributes", {{"counter", "0"}, {"_hidden", "true"}}},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "increase-counter"}, {"arguments", 
        "#sa general.counter++"}, {"permeable", true}}
    }},
  };

  // rooms
  const nlohmann::json ctx_room_1 = {
    {"id", "room_1"},
    {"name", "Room 1"},
    {"description", "Test room no. 1"},
    {"attributes", {{"gravity", "10"}, {"darkness", "99"}}},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "go to (.*)"}, {"ctx", "rooms/room_2"}, 
        {"arguments", "#ctx replace *rooms -> <ctx>"}, {"use_ctx_regex", UseCtx::NAME},
        {"permeable", false}},
      {{"id", "L2"}, {"re_event", "go to (.*)"}, {"ctx", "rooms/room_3"}, 
        {"arguments", "#ctx replace *rooms -> <ctx>"}, {"use_ctx_regex", UseCtx::NAME},
        {"permeable", false}}
    }},
  };

  const nlohmann::json ctx_room_2 = {
    {"id", "room_2"},
    {"name", "Room 2"},
    {"description", "Test room no. 2"},
    {"attributes", {{"gravity", "99"}, {"darkness", "9"}}}
  };

  const nlohmann::json ctx_room_3 = {
    {"id", "room_3"},
    {"name", "Room 3"},
    {"description", "Test room no. 3"},
    {"attributes", {{"gravity", "99"}, {"darkness", "9"}}}
  };

  // texts
  const nlohmann::json txt_text_1 = {
    {"txt", "Hello World"},
    {"permanent_events", "#print texts.random"},
    {"one_time_events", "#sa hp += 10"},
  };

  const nlohmann::json tests = nlohmann::json::array({
    {
      {"desc", "first-simple-test-case"}, 
      {"tests", {
        { {"cmd", "#> {*rooms->*rooms->name}"}, {"result", "Room 2, Room 3"} },
        { {"cmd", "go to Room 3"}, {"checks", {"{*rooms->name}=Room 3"}} },
        { {"cmd", "increase-counter"}, {"checks", {"{general.counter}=1"}} },
        { {"cmd", "increase-counter"}, {"checks", {"{general.counter}=2"}} }
      }}
    },
    {
      {"desc", "second-simple-test-case"}, 
      {"tests", {
        { {"cmd", "go to Room 3"}, {"checks", {"{*rooms->name}=Room 3"}} },
        { {"cmd", "increase-counter"}, {"checks", {"{general.counter}=1"}} }
      }}
    },
  });

  const std::string GAME_NAME = "test_game";
  const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
  test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}, {"rooms", {ctx_room_1, 
      ctx_room_2, ctx_room_3}}}, {{"texts/start", txt_text_1}}, tests);

  std::shared_ptr<BuilderGame> game = std::make_shared<BuilderGame>(GAME_PATH, GAME_NAME);

  // Run tests
  auto test_cases = test_parser::LoadTestCases(GAME_NAME); 
  for (const auto& test_case : test_cases) {
    REQUIRE(test_case.Run(game) == "");
  }
}

TEST_CASE("Store game", "[game]") {
  util::TmpPath tmp_path({txtad::GAME_FILES});
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
      {{"id", "L2"}, {"re_event", "go right"}, {"ctx", "rooms/room_2"}, 
        {"arguments", "#ctx replace *rooms -> <ctx>"}, 
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
        {"permeable", true}, {"use_ctx_regex", UseCtx::NAME}}
    }},
  };

  const nlohmann::json txt_text_1 = {
    {"txt", "Hello World"},
    {"permanent_events", "#print texts.random"},
    {"one_time_events", "#sa hp += 10"},
  };

  const std::string GAME_NAME = "test_game";
  const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
  test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}}, {"rooms", {ctx_room_1, ctx_room_2}}}, 
      {{"texts/start", txt_text_1}});
  BuilderGame game(GAME_PATH, GAME_NAME);

  // Store game 
  game.StoreGame(tmp_path.get());

  // Load game
  const std::string SAVED_GAME = "saved_game"; 
  Game saved_game(tmp_path.get(), SAVED_GAME); 

  // Test stored game
  REQUIRE(saved_game.name() == SAVED_GAME);
  REQUIRE(saved_game.path() == tmp_path.get()); 
  // Test settings created successfully
  REQUIRE(saved_game.settings().initial_events() == settings["initial_events"].get<std::string>());
  REQUIRE(saved_game.settings().initial_ctx_ids().size() == settings["initial_contexts"].size());
  REQUIRE(saved_game.settings().initial_ctx_ids().front() == "rooms/room_1");
  // Test contexts created successfully
  REQUIRE(saved_game.contexts().count("rooms/room_1") > 0); 
  auto ctx = saved_game.contexts().at("rooms/room_1");
  REQUIRE(ctx->id() == "rooms/" + ctx_room_1["id"].get<std::string>()); 
  REQUIRE(ctx->name() == ctx_room_1["name"].get<std::string>()); 
  // Tests attributes
  REQUIRE(ctx->GetAttribute("gravity") == ctx_room_1["attributes"]["gravity"].get<std::string>()); 
  REQUIRE(ctx->GetAttribute("darkness") == ctx_room_1["attributes"]["darkness"].get<std::string>()); 

  // Create user: 
  const std::string USER_ID = "0x1234";
  saved_game.HandleEvent(USER_ID, "");

  // Test events
  ExpressionParser parser;
  REQUIRE(ctx->TakeEvent("go left", parser) == false);
  REQUIRE(ctx->TakeEvent("go right", parser) == true);
  auto ctx_2 = saved_game.contexts().at("rooms/room_2");
  REQUIRE(ctx_2->TakeEvent("go left", parser) == false);
  REQUIRE(ctx_2->TakeEvent("go Room 1", parser) == true);

  // Test texts 
  REQUIRE(saved_game.texts().count("texts/start") > 0);
  std::string event_queue;
  REQUIRE(util::Join(saved_game.texts().at("texts/start")->print(event_queue, parser), ", ") == txt_text_1["txt"].get<std::string>());
  REQUIRE(event_queue == txt_text_1["permanent_events"].get<std::string>() + ";" 
      + txt_text_1["one_time_events"].get<std::string>());
  event_queue = "";
  // After second print, event_queue does not contain onetime events.
  REQUIRE(util::Join(saved_game.texts().at("texts/start")->print(event_queue, parser), ", ") == txt_text_1["txt"].get<std::string>());
  REQUIRE(event_queue == txt_text_1["permanent_events"].get<std::string>());
}

TEST_CASE("Test Game example dialog", "[game]") {
  const nlohmann::json settings = {
    {"initial_events", ""},
    {"initial_contexts", {"general", "dialogs/state_1"}}
  };

  const nlohmann::json ctx_general = {
    {"id", "general"},
    {"name", "General"},
    {"description", "Some general handlers"},
    {"permeable", true},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "increase-counter"}, {"arguments", 
        "#sa general.counter++"}, {"permeable", true}},
      {{"id", "L2"}, {"re_event", "#ctx replace *dialogs -> (.*)"}, {"arguments", 
        "#> {*dialogs->description}"}, {"permeable", true}}
    }},
  };

  const nlohmann::json dialog_state_1 = {
    {"id", "state_1"},
    {"name", "Dialog 1"},
    {"description", { 
      {"txt", "Hello sir, how are you?"},
      {"permanent_events", "#> Options:; #lst ctxs dialogs/state_1->*dialogs.preview"},
    }},
    {"listeners", {
      {{"id", "D1"}, {"re_event", "g"}, {"ctx", "dialogs/state_2"}, 
        {"arguments", "#ctx replace *dialogs -> <ctx>"}, {"use_ctx_regex", UseCtx::NO},
        {"permeable", false}},
      {{"id", "D2"}, {"re_event", "b"}, {"ctx", "dialogs/state_3"}, 
        {"arguments", "#ctx replace *dialogs -> <ctx>"}, {"use_ctx_regex", UseCtx::NO},
        {"permeable", false}}
    }},
  };

  const nlohmann::json dialog_state_2 = {
    {"id", "state_2"},
    {"name", "Dialog 2"},
    {"description", { 
      {"txt", "Oh that is splendid to here!"},
      {"permanent_events", ""},
    }},
    {"attributes", { {"preview", "[g]ood sir!"}}}
  };

  const nlohmann::json dialog_state_3 = {
    {"id", "state_3"},
    {"name", "Dialog 3"},
    {"description", { 
      {"txt", "Oh what a pity!"},
      {"permanent_events", ""},
    }},
    {"attributes", { {"preview", "[b]ad sir!"} } }
  };

  std::string cout = "";
  Game::set_global_msg_fn([&cout](std::string id, std::string txt) { 
      cout += ((cout != "") ? "\n" : "") + txt; });
  auto get_cout = [&cout]() { 
    std::string str = cout;
    cout = "";
    return str; 
  };

  const std::string GAME_NAME = "test_game_dialogs";
  const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
  test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}},
      {"dialogs", {dialog_state_1, dialog_state_2, dialog_state_3}}}, {});
  Game game(GAME_PATH, GAME_NAME);

  // Create users
  const std::string USER_ID = "0x1234";
  game.HandleEvent(USER_ID, "");

  game.HandleEvent(USER_ID, "#> {*dialogs->desc}");
  REQUIRE(get_cout() == "Hello sir, how are you?\nOptions:\n- [g]ood sir!\n- [b]ad sir!");
  game.HandleEvent(USER_ID, "g");
  REQUIRE(get_cout() == dialog_state_2["description"]["txt"].get<std::string>());
}

TEST_CASE("Test multi-edit", "[game]") {
  const nlohmann::json settings = {
    {"initial_events", ""},
    {"initial_contexts", {"general"}} 
  };

  const nlohmann::json ctx_general = {
    {"id", "general"},
    {"name", "General"},
    {"description", "Some general handlers"},
    {"permeable", true},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "increase-counter"}, {"arguments", 
        "#sa general.counter++"}, {"permeable", true}},
    }},
  };

  const nlohmann::json user_1 = {
    {"id", "user_1"},
    {"name", "User 1"},
    {"description", { {"txt", "A normal user"}, }},
    {"attributes", {{"life", "10"}}},
    {"listeners", nlohmann::json::array() },
  };
  const nlohmann::json user_2 = {
    {"id", "user_2"},
    {"name", "User 2"},
    {"description", { {"txt", "A normal user"}, }},
    {"attributes", {{"life", "20"}}},
    {"listeners", nlohmann::json::array() },
  };


  std::string cout = "";
  Game::set_global_msg_fn([&cout](std::string id, std::string txt) { 
      cout += ((cout != "") ? "\n" : "") + txt; });
  auto get_cout = [&cout]() { 
    std::string str = cout;
    cout = "";
    return str; 
  };

  const std::string GAME_NAME = "test_game_multiple";
  const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
  test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}},
      {"users", {user_1, user_2}}}, {});
  Game game(GAME_PATH, GAME_NAME);

  // Create users
  const std::string USER_ID = "0x1234";
  game.HandleEvent(USER_ID, "");

  game.HandleEvent(USER_ID, "#sa *users.life += 5");
  util::Logger()->info("User 1, life: {}", game.contexts().at("users/user_1")->GetAttribute("life").value());
  util::Logger()->info("User 2, life: {}", game.contexts().at("users/user_2")->GetAttribute("life").value());
  REQUIRE(game.contexts().at("users/user_1")->GetAttribute("life").value() == "10");
  REQUIRE(game.contexts().at("users/user_2")->GetAttribute("life").value() == "20");

  game.HandleEvent(USER_ID, "#ctx add users/user_1");
  game.HandleEvent(USER_ID, "#ctx add users/user_2");
  game.HandleEvent(USER_ID, "#sa *users.life += 5");
  util::Logger()->info("User 1, life: {}", game.contexts().at("users/user_1")->GetAttribute("life").value());
  util::Logger()->info("User 2, life: {}", game.contexts().at("users/user_2")->GetAttribute("life").value());
  REQUIRE(game.contexts().at("users/user_1")->GetAttribute("life").value() == "15");
  REQUIRE(game.contexts().at("users/user_2")->GetAttribute("life").value() == "25");
}

TEST_CASE("Test multi print", "[game]") {
  const nlohmann::json settings = {
    {"initial_events", ""},
    {"initial_contexts", {"general"}} 
  };

  const nlohmann::json ctx_general = {
    {"id", "general"},
    {"name", "General"},
    {"description", "Some general handlers"},
    {"permeable", true},
    {"listeners", {
      {{"id", "L1"}, {"re_event", "increase-counter"}, {"arguments", 
        "#sa general.counter++"}, {"permeable", true}},
    }},
  };

  const nlohmann::json user_1 = {
    {"id", "user_1"},
    {"name", "User 1"},
    {"description", { {"txt", "A normal user"}, }},
    {"attributes", {{"life", "10"}}},
    {"listeners", nlohmann::json::array() },
  };
  const nlohmann::json user_2 = {
    {"id", "user_2"},
    {"name", "User 2"},
    {"description", { {"txt", "A normal user"}, }},
    {"attributes", {{"life", "20"}}},
    {"listeners", nlohmann::json::array() },
  };


  std::string cout = "";
  Game::set_global_msg_fn([&cout](std::string id, std::string txt) { 
      cout += ((cout != "") ? "\n" : "") + txt; });
  auto get_cout = [&cout]() { 
    std::string str = cout;
    cout = "";
    return str; 
  };

  const std::string GAME_NAME = "test_game_multiple";
  const std::string GAME_PATH = txtad::GAMES_PATH + GAME_NAME;
  test::GameWrapper test_game_wrapper(GAME_NAME, settings, {{"", {ctx_general}},
      {"users", {user_1, user_2}}}, {});
  Game game(GAME_PATH, GAME_NAME);

  // Create users
  const std::string USER_ID = "0x1234";
  game.HandleEvent(USER_ID, "");

  game.HandleEvent(USER_ID, "#> {*users.life}");
  REQUIRE(cout == "");

  game.HandleEvent(USER_ID, "#> {**users.life}");
  REQUIRE(cout == "10;20");
}

