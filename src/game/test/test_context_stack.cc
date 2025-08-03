#include "game/test/helpers.h"
#include "shared/objects/context/context.h"
#include "shared/utils/defines.h"
#include "shared/utils/eventmanager/context_stack.h"
#include "shared/utils/eventmanager/listener.h"
#include "shared/utils/utils.h"
#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>
#include <memory>
#include <regex>

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
  ExpressionParser::SubstituteFN fn = [&attributes](const std::string& str) { 
    return (attributes.count(str) > 0) ? attributes.at(str) : ""; };
  ExpressionParser parser(fn);
  std::string event_queue = "";

  ContextStack stack;
  std::map<std::string, std::shared_ptr<Context>> contexts;
  
  // define basic handlers
  Listener::Fn set_attribute = [&attributes, &parser](std::string event, std::string args) {
    helpers::SetAttribute(attributes, args, parser);
  };
  
  Listener::Fn add_ctx = [&contexts, &stack](std::string event, std::string ctx_id) {
    util::Logger()->info("Adding context: {}", ctx_id);
    if (contexts.count(ctx_id) > 0)
      stack.insert(contexts.at(ctx_id));
  };
  Listener::Fn remove_ctx = [&contexts, &stack](std::string event, std::string ctx_id) {
    util::Logger()->info("Removing context: {}", ctx_id);
    if (stack.exists(ctx_id))
      stack.erase(ctx_id);
  };
  Listener::Fn replace_ctx = [&contexts, &stack](std::string event, std::string args) {
    static const std::regex regex_expression("(.*) -> (.*)");
    std::smatch base_match;
    if (std::regex_match(args, base_match, regex_expression)) {
      if (base_match.size() == 3) {
        std::string ctx_from = base_match[1].str();
        std::string ctx_to = base_match[2].str();
        util::Logger()->info("Replacing context: {} with {}", ctx_from, ctx_to);
        if (stack.exists(ctx_from))
          stack.erase(ctx_from);
        if (contexts.count(ctx_to) > 0)
          stack.insert(contexts.at(ctx_to));
      }
    }
  };

  Listener::Fn cout = [&event_queue](std::string event, std::string args) {
    util::Logger()->info("PRINT: >>{}<<", args);
  };

  Listener::Fn add_to_eventqueue = [&event_queue](std::string event, std::string args) {
    event_queue += ((event_queue != "") ? ";" : "") + args;
  };

  LForwarder::set_overwite_fn(add_to_eventqueue);


  // Create Base context
  std::shared_ptr<Context> ctx = std::make_shared<Context>("ctx_mechanic", 10);
  ctx->AddListener(std::make_shared<LHandler>("H1", "#ctx remove (.*)", remove_ctx));
  ctx->AddListener(std::make_shared<LHandler>("H2", "#ctx add (.*)", add_ctx));
  ctx->AddListener(std::make_shared<LHandler>("H3", "#ctx replace (.*)", replace_ctx));
  ctx->AddListener(std::make_shared<LHandler>("H4", "#print (.*)", cout));
  contexts[ctx->id()] = ctx;
  stack.insert(ctx);

  // Create context "scared"
  std::shared_ptr<Context> ctx_scared = std::make_shared<Context>("scared", 0);
  ctx_scared->AddListener(std::make_shared<LForwarder>("L1", "go west", "#ctx add mindfullness;#ctx remove scared", true));
  contexts[ctx_scared->id()] = ctx_scared;
  stack.insert(ctx_scared);

  // Create context "mindfullness"
  std::shared_ptr<Context> ctx_mindfullness = std::make_shared<Context>("mindfullness", 0);
  ctx_mindfullness->AddListener(std::make_shared<LForwarder>("L1", "go east", "#ctx replace mindfullness -> scared", true));
  contexts[ctx_mindfullness->id()] = ctx_mindfullness;

  // "ctx_mechanic" and "scared" or initial contexts
  REQUIRE(stack.exists("ctx_mechanic"));
  REQUIRE(stack.exists("scared"));
  REQUIRE(!stack.exists("mindfullness"));

  SECTION("Switch states") {
    // Switch from scared to mindfullness
    event_queue = "go west";
    while (event_queue != "") {
      stack.TakeEvents(event_queue, parser);
    }
    REQUIRE(!stack.exists("scared"));
    REQUIRE(stack.exists("mindfullness"));
    // Switch back to scared 
    event_queue = "go east";
    while (event_queue != "") {
      stack.TakeEvents(event_queue, parser);
    }
    REQUIRE(stack.exists("scared"));
    REQUIRE(!stack.exists("mindfullness"));
  }

  SECTION("Test priority and permeability") {
    // Create Curse context
    std::shared_ptr<Context> ctx_curse = std::make_shared<Context>("curse", 5, false);
    ctx_curse->AddListener(std::make_shared<LForwarder>("L1", "go west", "#print You are cursed and can't go west", true));
    contexts[ctx_curse->id()] = ctx_mindfullness;
    stack.insert(ctx_curse);

    // Attempt to go west
    event_queue = "go west";
    while (event_queue != "") {
      stack.TakeEvents(event_queue, parser);
    }
    // Nothing changed, becuse curse prevents going west
    REQUIRE(stack.exists("scared"));
    REQUIRE(!stack.exists("mindfullness"));
  }
}

TEST_CASE("Test throwing events (with context-listener)", "[stack]") {
  std::map<std::string, std::string> attributes = {{"mana", "10"}, {"runes", "5"}};
  ExpressionParser::SubstituteFN fn = [&attributes](const std::string& str) { 
    return (attributes.count(str) > 0) ? attributes.at(str) : ""; };
  ExpressionParser parser(fn);
  std::string event_queue = "";

  ContextStack stack;
  std::map<std::string, std::shared_ptr<Context>> contexts;
  
  // difine basic handlers
  Listener::Fn set_attribute = [&attributes, &parser](std::string event, std::string args) {
    helpers::SetAttribute(attributes, args, parser);
  };
  
  Listener::Fn add_ctx = [&contexts, &stack](std::string event, std::string ctx_id) {
    util::Logger()->info("Adding context: {}", ctx_id);
    if (contexts.count(ctx_id) > 0)
      stack.insert(contexts.at(ctx_id));
    else 
      util::Logger()->error("[add_ctx] failed: ctx '{}' not found!", ctx_id);
  };
  Listener::Fn remove_ctx = [&contexts, &stack](std::string event, std::string ctx_id) {
    util::Logger()->info("Removing context: {}", ctx_id);
    if (ctx_id.front() == '*') {
      for (const auto& ctx : stack.find(ctx_id.substr(1))) 
        stack.erase(ctx->id());
      return;
    } else if (stack.exists(ctx_id))
      stack.erase(ctx_id);
    else 
      util::Logger()->error("[remove_ctx] failed: ctx '{}' not found!", ctx_id);
  };
  Listener::Fn replace_ctx = [&contexts, &stack, &add_ctx, &remove_ctx](std::string event, std::string args) {
    static const std::regex regex_expression("(.*) -> (.*)");
    std::smatch base_match;
    if (std::regex_match(args, base_match, regex_expression)) {
      if (base_match.size() == 3) {
        std::string ctx_from = base_match[1].str();
        std::string ctx_to = base_match[2].str();
        util::Logger()->info("Replacing context: {} with {}", ctx_from, ctx_to);
        // Get context to add: 
        remove_ctx("", ctx_from);
        add_ctx("", ctx_to);
      }
    }
  };

  Listener::Fn cout = [&event_queue](std::string event, std::string args) {
    util::Logger()->info("PRINT: >>{}<<", args);
  };

  Listener::Fn add_to_eventqueue = [&event_queue](std::string event, std::string args) {
    event_queue += ((event_queue != "") ? ";" : "") + args;
  };

  LForwarder::set_overwite_fn(add_to_eventqueue);


  // Create Base context
  std::shared_ptr<Context> ctx = std::make_shared<Context>("ctx_mechanic", 10);
  ctx->AddListener(std::make_shared<LHandler>("H1", "#ctx remove (.*)", remove_ctx));
  ctx->AddListener(std::make_shared<LHandler>("H2", "#ctx add (.*)", add_ctx));
  ctx->AddListener(std::make_shared<LHandler>("H3", "#ctx replace (.*)", replace_ctx));
  ctx->AddListener(std::make_shared<LHandler>("H4", "#print (.*)", cout));
  contexts[ctx->id()] = ctx;
  stack.insert(ctx);

  // Create contexts 
  std::shared_ptr<Context> ctx_scared = std::make_shared<Context>("rooms.scared", "Scared", "", "");
  contexts[ctx_scared->id()] = ctx_scared;
  std::shared_ptr<Context> ctx_mindfullness = std::make_shared<Context>("rooms.mindfullness", "Mindfullness", "", "west");
  contexts[ctx_mindfullness->id()] = ctx_mindfullness;


  // Add listeners to context "scared"
  ctx_scared->AddListener(std::make_shared<LContextForwarder>("L1", "go (.*)", ctx_mindfullness, 
        "#ctx remove *rooms;#ctx add <ctx>", true, UseCtx::REGEX));
  
  // Add listeners to context "mindfullness"
  ctx_mindfullness->AddListener(std::make_shared<LContextForwarder>("L1", "go (.*)", ctx_scared, 
        "#ctx replace *rooms -> <ctx>", true, UseCtx::NAME));


  // Set initial contexts
  stack.insert(ctx_scared);

  // "ctx_mechanic" and "scared" or initial contexts
  REQUIRE(stack.exists("ctx_mechanic"));
  REQUIRE(stack.exists("rooms.scared"));
  REQUIRE(!stack.exists("rooms.mindfullness"));

  SECTION("Switch states") {
    // Switch from scared to mindfullness
    event_queue = "go west";
    while (event_queue != "") {
      stack.TakeEvents(event_queue, parser);
    }
    REQUIRE(!stack.exists("rooms.scared"));
    REQUIRE(stack.exists("rooms.mindfullness"));
    // Switch back to scared 
    event_queue = "go scared";
    while (event_queue != "") {
      stack.TakeEvents(event_queue, parser);
    }
    REQUIRE(stack.exists("rooms.scared"));
    REQUIRE(!stack.exists("rooms.mindfullness"));
  }

  SECTION("Test priority and permeability") {
    // Create Curse context
    std::shared_ptr<Context> ctx_curse = std::make_shared<Context>("curse", 5, false);
    ctx_curse->AddListener(std::make_shared<LForwarder>("L1", "go west", "#print You are cursed and can't go west", true));
    contexts[ctx_curse->id()] = ctx_mindfullness;
    stack.insert(ctx_curse);

    // Attempt to go west
    event_queue = "go west";
    while (event_queue != "") {
      stack.TakeEvents(event_queue, parser);
    }
    // Nothing changed, becuse curse prevents going west
    REQUIRE(stack.exists("rooms.scared"));
    REQUIRE(!stack.exists("rooms.mindfullness"));
  }
}
