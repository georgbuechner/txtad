#include "shared/objects/context/context.h"
#include <catch2/catch_test_macros.hpp>
                                                                             

TEST_CASE("Test simple use", "[objects, context]") {
  const std::string ID = "_1";
  const std::string NAME = "test";
  const std::string DESCRIPTION = "test context";

  Context ctx(ID, NAME, DESCRIPTION, "", 0, false);
  REQUIRE(ctx.id() == ID);
  REQUIRE(ctx.name() == NAME);
  REQUIRE(ctx.description() == DESCRIPTION);
}

// test Attribute Methods
/* add a new attribute
   add a duplicate
   set existing attribute
   set non-existing attribute
   get known key
   get unknown key
   remove attribute
   check true/false cases for 'Has Attribute'
*/
TEST_CASE("Test attributes", "[objects, context]") {

}

/*
// test Getters 
TEST_CASE("Test Getters", "[name]") {
  Context name = name();
  REQUIRE(name.Evaluate("kitchen") == "kitchen");
}// check for const also?

TEST_CASE("Test Getters", "[description]") {
  Context description = description();
  REQUIRE(description.Evaluate("a place for cooking") == "a place for cooking");
}// check for const also?

TEST_CASE("Test Getters", "[entry_condition_pattern]") {
  Context entry_condition_pattern = entry_condition_pattern();
  REQUIRE(entry_condition_pattern.Evaluate("enter kitchen") == "enter kitchen");
  REQUIRE(entry_condition_pattern.Evaluate("^enter *$") == "^enter *$");
}  // check for const also?

// test Setters
TEST_CASE("Test Setters", "[set_name]") {
  Context set_name = set_name();
  REQUIRE(set_name.Evaluate("bathroom") == "bathroom");
}

TEST_CASE("Test Setters", "[set_description]") {
  Context set_description = set_description();
  REQUIRE(set_description.Evaluate("a place for washing") == "a place for washing");
}

TEST_CASE("Test Setters", "[set_entry_condition]") {
  Context set_entry_condition = set_entry_condition();
  REQUIRE(set_entry_condition.Evaluate("enter bathroom") == "enter bathroom");
  REQUIRE(set_entry_condition.Evaluate("xxx to do xxx") == "xxx to do xxx"); //std::regex
}
*/

// test Entry Check

// test Listener methods calling EventManager
