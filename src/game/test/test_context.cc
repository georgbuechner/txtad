#include "shared/objects/context/context.h"
#include <catch2/catch_test_macros.hpp>
#include <map>

// constants used across different tests
const std::string ID = "_1";
const std::string NAME = "some name";
const std::string DESCRIPTION = "some description";
const std::string ENTRY_CONDITION_PATTERN = "abc.*";
const int PRIORITY = 4;
const bool PERMEABLE = false;

TEST_CASE("Test constructor", "[objects][context][constructor]") {
  
  SECTION("Test minimal constructor") {
      Context ctx(ID, PRIORITY, PERMEABLE);

      REQUIRE(ctx.id() == ID);
      REQUIRE(ctx.priority() == PRIORITY);
      REQUIRE(ctx.permeable() == PERMEABLE);
  }

  SECTION("Test full constructor") {
      Context ctx(ID, NAME, DESCRIPTION, ENTRY_CONDITION_PATTERN, PRIORITY, PERMEABLE);

      REQUIRE(ctx.id() == ID);
      REQUIRE(ctx.name() == NAME);
      REQUIRE(ctx.description() == DESCRIPTION);
      REQUIRE(ctx.entry_condition_pattern() == ENTRY_CONDITION_PATTERN);
      REQUIRE(ctx.priority() == PRIORITY);
      REQUIRE(ctx.permeable() == PERMEABLE);
  }
}

TEST_CASE("Test getters", "[objects][context][getters]") {
  Context ctx(ID, NAME, DESCRIPTION, ENTRY_CONDITION_PATTERN, PRIORITY, PERMEABLE);

  REQUIRE(ctx.id() == ID);
  REQUIRE(ctx.name() == NAME);
  REQUIRE(ctx.description() == DESCRIPTION);
  REQUIRE(ctx.entry_condition_pattern() == ENTRY_CONDITION_PATTERN);
  REQUIRE(ctx.priority() == PRIORITY);
  REQUIRE(ctx.permeable() == PERMEABLE);
}

TEST_CASE("Test setters", "[objects][context][setters]") {
  Context ctx(ID, NAME, DESCRIPTION, ENTRY_CONDITION_PATTERN, PRIORITY, PERMEABLE);

  SECTION("Test set_name") {
    const std::string NEW_NAME = "some new name";
    ctx.set_name(NEW_NAME);
    REQUIRE(ctx.name() != NAME);
    REQUIRE(ctx.name() == NEW_NAME);
  }

  SECTION("Test set_description") {
    const std::string NEW_DESCRIPTION = "some new description";
    ctx.set_description(NEW_DESCRIPTION);
    REQUIRE(ctx.description() != DESCRIPTION);
    REQUIRE(ctx.description() == NEW_DESCRIPTION);
  }

  SECTION("Test set_entry_condition") {
    const std::string NEW_ENTRY_CONDITION_PATTERN = "xyz.*";
    ctx.set_entry_condition(NEW_ENTRY_CONDITION_PATTERN);
    REQUIRE(ctx.entry_condition_pattern() != ENTRY_CONDITION_PATTERN);
    REQUIRE(ctx.entry_condition_pattern() == NEW_ENTRY_CONDITION_PATTERN);
  }
}

TEST_CASE("Test string representation of the class", "[objects][context][string representation]") {
  Context ctx(ID, NAME, DESCRIPTION, ENTRY_CONDITION_PATTERN, PRIORITY, PERMEABLE);

  std::string str = ctx.ToString();

  REQUIRE(str.find("Name: " + NAME) != std:string::npos);
  REQUIRE(str.find("Description: " + DESCRIPTION) != std:string::npos);
  REQUIRE(str.find("Entry Condition (regex): " + ENTRY_CONDITION_PATTERN) != std:string::npos);
}

TEST_CASE("Test entry check", "[objects][context][entry check]") {
  Context ctx(ID, NAME, DESCRIPTION, ENTRY_CONDITION_PATTERN, PRIORITY, PERMEABLE);

  SECTION("Matching input returns true") {
    REQUIRE(ctx.CheckEntry("abc123"));
  }

  SECTION("Matching input returns false") {
    REQUIRE_FALSE(ctx.CheckEntry("ab"));
  }
}

TEST_CASE("Test attribute methods", "[objects][context][attribute methods]") {
  Context ctx(ID, NAME, DESCRIPTION, ENTRY_CONDITION_PATTERN, PRIORITY, PERMEABLE);

  SECTION("Add new attribute") {
    REQUIRE(ctx.AddAttribute("key1", "value1") == true);
  }

  SECTION("Adding a duplicate attribute fails") {
    ctx.AddAttribute("key1", "value1");
    REQUIRE(ctx.AddAttribute("key1", "value1") == false);
  }

  SECTION("Set new attribute") {
    REQUIRE(ctx.SetAttribute("key2", "value2") == true);
    REQUIRE(ctx.GetAttribute("key2").value_or("") == "value2");
  }

  SECTION("Setting an existing attribute fails") {
    ctx.AddAttribute("key1", "value1");
    REQUIRE(ctx.SetAttribute("key1", "new_value") == false);
    REQUIRE(ctx.GetAttribute("key1").value_or("") == "value1");
  }

  SECTION("Get known key") {
    ctx.AddAttribute("key3", "value3");
    auto val = ctx.GetAttribute("key3");
    REQUIRE(val.has_value());
    REQUIRE(ctx.GetAttribute("key3") == "value3");
  }

  SECTION("Getting unknown key returns nullopt") {
    REQUIRE_FALSE(ctx.GetAttribute("key4").has_value());
  }

  SECTION("Remove existing attribute") {
    ctx.AddAttribute("key5", "value5");
    REQUIRE(ctx.RemoveAttribute("key5") == true);
  }

  SECTION("Remove non-existing attribute") {
    REQUIRE(ctx.RemoveAttribute("key6") == false);
  }

  SECTION("Has attribute") {
    ctx.AddAttribute("key7", "value7");
    REQUIRE(ctx.HasAttribute("key7") == true);
  }

  SECTION("Has no attribute") {
    REQUIRE(ctx.HasAttribute("key7") == false);
  }
}

TEST_CASE("Test listener methods calling EventManager", "[objects][context][listener methods]") {
  Context ctx(ID, NAME, DESCRIPTION, ENTRY_CONDITION_PATTERN, PRIORITY, PERMEABLE);

  ExpressionParser PARSER;
  
  SECTION("TakeEvent works") {
    const std::string EVENT = "some event";
    
    bool result = ctx.TakeEvent(EVENT, PARSER);

    REQUIRE(result);
  }
  
  SECTION("AddListener does not throw") {
    class DummyListener : public Listener {
    public:
      std::string id() const override { return "dummy"; }
    };

    auto listener = std::make_shared<DummyListener>();
    
    REQUIRE_NOTHROW(ctx.AddListener(listener));
  }

  SECTION("RemoveListener does not throw") {
    REQUIRE_NOTHROW(ctx.RemoveListener(ID));
  }
}
