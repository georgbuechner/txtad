#include "game/test/helpers.h"
#include "shared/objects/context/context.h"
#include "shared/utils/eventmanager/context_stack.h"
#include "shared/utils/eventmanager/eventmanager.h"
#include "shared/utils/utils.h"
#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>
#include <memory>

TEST_CASE("Test inserting, removing, getting and sorting", "[stack]") {
  ContextStack stack;

  SECTION("higher priority first") {
    REQUIRE(stack.insert(std::make_shared<Context>("1", 0)));
    REQUIRE(stack.insert(std::make_shared<Context>("2", -1)));
    const auto& sorted_ctx_ids = stack.GetOrder();
    auto pos_1 = std::find(sorted_ctx_ids.begin(), sorted_ctx_ids.end(), "1");
    auto pos_2 = std::find(sorted_ctx_ids.begin(), sorted_ctx_ids.end(), "2");
    REQUIRE((pos_1 != sorted_ctx_ids.end() && pos_2 != sorted_ctx_ids.end()));
    REQUIRE(pos_1 < pos_2);
  }

  SECTION("lower priority first") {
    REQUIRE(stack.insert(std::make_shared<Context>("1", 0)));
    REQUIRE(stack.insert(std::make_shared<Context>("2", 1)));
    const auto& sorted_ctx_ids = stack.GetOrder();
    auto pos_1 = std::find(sorted_ctx_ids.begin(), sorted_ctx_ids.end(), "1");
    auto pos_2 = std::find(sorted_ctx_ids.begin(), sorted_ctx_ids.end(), "2");
    REQUIRE((pos_1 != sorted_ctx_ids.end() && pos_2 != sorted_ctx_ids.end()));
    REQUIRE(pos_1 > pos_2);
  }

  SECTION("insert multiple priority first") {
    REQUIRE(stack.insert(std::make_shared<Context>("1", 0)));
    REQUIRE(stack.insert(std::make_shared<Context>("2", 1)));
    REQUIRE(stack.insert(std::make_shared<Context>("3", -2)));
    REQUIRE(stack.insert(std::make_shared<Context>("4", -3)));
    REQUIRE(stack.insert(std::make_shared<Context>("5", 2)));
    REQUIRE(stack.insert(std::make_shared<Context>("6", -1)));
    const auto& sorted_ctx_ids = stack.GetOrder();
    const std::vector<std::string> expected_order = {"5", "2", "1", "6", "3", "4"};
    REQUIRE(sorted_ctx_ids == expected_order);
  }

  SECTION("Adding existing") {
    REQUIRE(stack.insert(std::make_shared<Context>("1", 0)) == true);
    REQUIRE(stack.insert(std::make_shared<Context>("1", 1)) == false);
    REQUIRE(stack.GetOrder().size() == 1);
  }

  SECTION("Removing context") {
    REQUIRE(stack.erase("1") == false);
    REQUIRE(stack.insert(std::make_shared<Context>("1", 0)));
    REQUIRE(stack.erase("1") == true);
    REQUIRE(stack.GetOrder().size() == 0);
  }

  SECTION("Getting context") {
    REQUIRE(stack.get("1") == nullptr);
    REQUIRE(stack.insert(std::make_shared<Context>("1", 0)));
    REQUIRE(stack.get("1") != nullptr);
    if (const auto& context = stack.get("1"))
      REQUIRE(context->id() == "1");
    else 
      REQUIRE(false);
  }
}

TEST_CASE("Test throwing events", "[stack]") {
  std::map<std::string, std::string> attributes = {{"mana", "10"}, {"runes", "5"}};
  ExpressionParser parser(&attributes);
  std::string event_queue = "";

  ContextStack stack;
  std::map<std::string, std::shared_ptr<Context>> contexts;
  
  // difine basic handlers
  Listener::Fn set_attribute = [&attributes](std::string event, std::string args) {
    helpers::SetAttribute(attributes, args);
  };
  
  Listener::Fn add_ctx = [&contexts, &stack](std::string event, std::string args) {
    util::Logger()->info(fmt::format("Adding context: {}", args));
    if (contexts.count(args) > 0)
      stack.insert(contexts.at(args));
  };
  Listener::Fn remove_ctx = [&contexts, &stack](std::string event, std::string args) {
    util::Logger()->info(fmt::format("Removing context: {}", args));
    if (contexts.count(args) > 0)
      stack.erase(args);
  };

  Listener::Fn add_to_eventqueue = [&event_queue](std::string event, std::string args) {
    event_queue += ((event_queue != "") ? ";" : "") + args;
  };

  // Create Base context
  std::shared_ptr<Context> ctx = std::make_shared<Context>("ctx_mechanic", 10);
  ctx->AddListener(std::make_shared<Listener>("H1", "#ctx remove (.*)", "", remove_ctx, true));
  ctx->AddListener(std::make_shared<Listener>("H2", "#ctx add (.*)", "", add_ctx, true));
  contexts[ctx->id()] = ctx;
  stack.insert(ctx);

  // Create context "scared"
  std::shared_ptr<Context> ctx_scared = std::make_shared<Context>("scared", 0);
  ctx_scared->AddListener(std::make_shared<Listener>("L1", "go west", "#ctx add mindfullness;#ctx remove scared",
      add_to_eventqueue, true));
  contexts[ctx_scared->id()] = ctx_scared;
  stack.insert(ctx_scared);

  // Create context "mindfullness"
  std::shared_ptr<Context> ctx_mindfullness = std::make_shared<Context>("mindfullness", 0);
  ctx_mindfullness->AddListener(std::make_shared<Listener>("L1", "go east", "#ctx add scared;#ctx remove mindfullness",
      add_to_eventqueue, true));
  contexts[ctx_mindfullness->id()] = ctx_mindfullness;

  // "ctx_mechanic" and "scared" or initial contexts
  REQUIRE(stack.get("ctx_mechanic") != nullptr);
  REQUIRE(stack.get("scared") != nullptr);
  REQUIRE(stack.get("mindfullness") == nullptr);

  SECTION("Switch rooms") {
    // Switch from scared to mindfullness
    event_queue = "go west";
    while (event_queue != "") {
      stack.TakeEvents(event_queue, parser);
    }
    REQUIRE(stack.get("scared") == nullptr);
    REQUIRE(stack.get("mindfullness") != nullptr);
    // Switch back to scared 
    event_queue = "go east";
    while (event_queue != "") {
      stack.TakeEvents(event_queue, parser);
    }
    REQUIRE(stack.get("scared") != nullptr);
    REQUIRE(stack.get("mindfullness") == nullptr);
  }
}
