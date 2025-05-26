#include "shared/objects/context/context.h"
#include "shared/utils/eventmanager/context_stack.h"
#include "shared/utils/eventmanager/eventmanager.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>

std::shared_ptr<Context> CreateSimpleContext(std::string id, int priority) {
  return std::make_shared<Context>(id, "", "", "", priority, true, new EventManager());

}

TEST_CASE("Test inserting, removing, getting and sorting", "[stack]") {
  ContextStack stack;

  SECTION("higher priority first") {
    REQUIRE(stack.insert(CreateSimpleContext("1", 0)));
    REQUIRE(stack.insert(CreateSimpleContext("2", -1)));
    const auto& sorted_ctx_ids = stack.GetOrder();
    auto pos_1 = std::find(sorted_ctx_ids.begin(), sorted_ctx_ids.end(), "1");
    auto pos_2 = std::find(sorted_ctx_ids.begin(), sorted_ctx_ids.end(), "2");
    REQUIRE((pos_1 != sorted_ctx_ids.end() && pos_2 != sorted_ctx_ids.end()));
    REQUIRE(pos_1 < pos_2);
          
  }

  SECTION("lower priority first") {
    REQUIRE(stack.insert(CreateSimpleContext("1", 0)));
    REQUIRE(stack.insert(CreateSimpleContext("2", 1)));
    const auto& sorted_ctx_ids = stack.GetOrder();
    auto pos_1 = std::find(sorted_ctx_ids.begin(), sorted_ctx_ids.end(), "1");
    auto pos_2 = std::find(sorted_ctx_ids.begin(), sorted_ctx_ids.end(), "2");
    REQUIRE((pos_1 != sorted_ctx_ids.end() && pos_2 != sorted_ctx_ids.end()));
    REQUIRE(pos_1 > pos_2);
  }

  SECTION("insert multiple priority first") {
    REQUIRE(stack.insert(CreateSimpleContext("1", 0)));
    REQUIRE(stack.insert(CreateSimpleContext("2", 1)));
    REQUIRE(stack.insert(CreateSimpleContext("3", -2)));
    REQUIRE(stack.insert(CreateSimpleContext("4", -3)));
    REQUIRE(stack.insert(CreateSimpleContext("5", 2)));
    REQUIRE(stack.insert(CreateSimpleContext("6", -1)));
    const auto& sorted_ctx_ids = stack.GetOrder();
    const std::vector<std::string> expected_order = {"5", "2", "1", "6", "3", "4"};
    REQUIRE(sorted_ctx_ids == expected_order);
  }

  SECTION("Adding existing") {
    REQUIRE(stack.insert(CreateSimpleContext("1", 0)));
    REQUIRE(stack.insert(CreateSimpleContext("1", 1)) == false);
    REQUIRE(stack.GetOrder().size() == 1);
  }

  SECTION("Removing context") {
    REQUIRE(stack.erase("1") == false);
    REQUIRE(stack.insert(CreateSimpleContext("1", 0)));
    REQUIRE(stack.erase("1") == true);
    REQUIRE(stack.GetOrder().size() == 0);
  }

  SECTION("Getting context") {
    REQUIRE(stack.get("1") == nullptr);
    REQUIRE(stack.insert(CreateSimpleContext("1", 0)));
    REQUIRE(stack.get("1") != nullptr);
    if (const auto& context = stack.get("1"))
      REQUIRE(context->id() == "1");
    else 
      REQUIRE(false);
  }
}
