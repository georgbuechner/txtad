cmake_minimum_required(VERSION 3.23)

project(txtad)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)                                                                          
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "-Wall -pthread -lcurl -O3")
add_compile_options (-fdiagnostics-color=always)
add_compile_options(-w)


set(GAME_SRC_FILES 
  src/game/main.cc 
  src/game/game/game.cc 
  src/game/game/user.cc 
  src/game/server/websocket_server.cc 
  src/shared/objects/text/text.cc
  src/shared/objects/context/context.cc
  src/shared/utils/eventmanager/context_stack.cc
  src/shared/utils/eventmanager/listener.cc
  src/shared/utils/utils.cc
  src/shared/utils/parser/expression_parser.cc
  src/shared/utils/parser/game_file_parser.cc
  src/shared/utils/parser/pattern_parser.cc
  src/shared/utils/fuzzy_search/fuzzy.cc
)

set(GAME_TEST_FILES 
  src/game/test/main.cc 
  src/game/test/test_context.cc
  src/game/test/test_context_stack.cc
  src/game/test/test_eventmanager.cc
  src/game/test/test_expression_parser.cc
  src/game/test/test_game.cc
  src/game/test/test_game_file_parser.cc
  src/game/test/test_text.cc
  src/game/test/test_utils.cc
  src/game/test/helpers.cc
  src/game/game/game.cc
  src/game/game/user.cc
  src/builder/game/builder_game.cc
  src/shared/objects/tests/test_case.cc
  src/shared/objects/tests/test.cc
  src/shared/objects/context/context.cc
  src/shared/objects/text/text.cc
  src/shared/utils/eventmanager/context_stack.cc
  src/shared/utils/eventmanager/listener.cc
  src/shared/utils/utils.cc
  src/shared/utils/parser/expression_parser.cc
  src/shared/utils/parser/game_file_parser.cc
  src/shared/utils/parser/test_file_parser.cc
  src/shared/utils/parser/pattern_parser.cc
  src/shared/utils/fuzzy_search/fuzzy.cc
)

set(BUILDER_SRC_FILES 
  src/builder/main.cc 
  src/builder/server/server.cc
  src/builder/utils/http_helpers.cc
  src/builder/utils/jinja_helpers.cc
  src/builder/game/builder_game.cc
  src/game/game/game.cc 
  src/game/game/user.cc 
  src/shared/objects/context/context.cc
  src/shared/objects/tests/test.cc
  src/shared/objects/tests/test_case.cc
  src/shared/objects/text/text.cc
  src/shared/utils/eventmanager/context_stack.cc
  src/shared/utils/eventmanager/listener.cc
  src/shared/utils/utils.cc
  src/shared/utils/parser/expression_parser.cc
  src/shared/utils/parser/game_file_parser.cc
  src/shared/utils/parser/test_file_parser.cc
  src/shared/utils/parser/pattern_parser.cc
  src/shared/utils/fuzzy_search/fuzzy.cc
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(websocketpp CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)

# Jinja2 dependencies

# Make sure cache options from parent are honored
cmake_policy(SET CMP0077 NEW)

include(FetchContent)

find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED)               # header-only use is fine; vcpkg picks a version
find_package(expected-lite CONFIG REQUIRED)
find_package(variant-lite CONFIG REQUIRED)
find_package(optional-lite CONFIG REQUIRED)
find_package(string-view-lite CONFIG REQUIRED)


# --- Compatibility shims: map vcpkg target names to what Jinja2Cpp expects ---
# martinmoene nonstd::*  ->  *-lite::*-lite
if(TARGET nonstd::expected-lite AND NOT TARGET expected-lite::expected-lite)
  add_library(expected-lite::expected-lite ALIAS nonstd::expected-lite)
endif()

if(TARGET nonstd::optional-lite AND NOT TARGET optional-lite::optional-lite)
  add_library(optional-lite::optional-lite ALIAS nonstd::optional-lite)
endif()

if(TARGET nonstd::variant-lite AND NOT TARGET variant-lite::variant-lite)
  add_library(variant-lite::variant-lite ALIAS nonstd::variant-lite)
endif()

if(TARGET nonstd::string-view-lite AND NOT TARGET string-view-lite::string-view-lite)
  add_library(string-view-lite::string-view-lite ALIAS nonstd::string-view-lite)
endif()

# fmt: Jinja2Cpp may look for fmtlib::fmt; vcpkg exports fmt::fmt
if(TARGET fmt::fmt AND NOT TARGET fmtlib::fmt)
  add_library(fmtlib::fmt ALIAS fmt::fmt)
endif()

# Build Jinja2Cpp against external deps (provided by vcpkg)
set(JINJA2CPP_DEPS_MODE external CACHE STRING "" FORCE)

FetchContent_Declare(
  jinja2cpp
  GIT_REPOSITORY https://github.com/jinja2cpp/Jinja2Cpp.git
  GIT_TAG        1.3.2           # pin a release tag
  GIT_SHALLOW    TRUE           
)
FetchContent_MakeAvailable(jinja2cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/game)
add_executable(txtad.o ${GAME_SRC_FILES})
add_executable(txtad_test.o ${GAME_TEST_FILES})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/builder)
add_executable(builder.o ${BUILDER_SRC_FILES})

add_compile_definitions(COMPILE_SPI_COMMANDS_FOR_CXX=1)
add_compile_definitions(JSON_DIAGNOSTICS=1)
if (COMPILE_FOR_SERVER)
  message("compiling for server...")
  target_compile_definitions(txtad.o PUBLIC _COMPILE_FOR_SERVER_)
  target_compile_definitions(builder.o PUBLIC _COMPILE_FOR_SERVER_)
else()
  message("compiling for localhost...")
endif()

target_link_libraries(txtad.o PRIVATE
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  websocketpp::websocketpp 
  CURL::libcurl
)

target_link_libraries(txtad_test.o PRIVATE
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  websocketpp::websocketpp 
  Catch2::Catch2
  CURL::libcurl
)

target_link_libraries(builder.o PRIVATE
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  CURL::libcurl
  jinja2cpp 
  fmt::fmt
)

target_include_directories(txtad.o PUBLIC "src" "src/shared/libraries")
target_include_directories(txtad_test.o PUBLIC "src" "src/shared/libraries")
target_include_directories(builder.o PUBLIC "src" "src/shared/libraries")
