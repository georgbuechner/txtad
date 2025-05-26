cmake_minimum_required(VERSION 3.23)

project(txtad)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)                                                                          
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "-Wall -pthread -O3")
add_compile_options (-fdiagnostics-color=always)
add_compile_options(-w)

set(GAME_SRC_FILES 
  src/game/main.cc 
  src/game/game/game.cc 
  src/game/game/user.cc 
  src/game/server/websocket_server.cc 
  src/shared/objects/context/context.cc
  src/shared/utils/eventmanager/context_stack.cc
  src/shared/utils/utils.cc
  src/shared/utils/parser/expression_parser.cc
  src/shared/utils/fuzzy_search/fuzzy.cc
)

set(GAME_TEST_FILES 
  src/game/test/main.cc 
  src/game/test/test_eventmanager.cc
  src/game/test/test_context_stack.cc
  src/game/test/test_expression_parser.cc
  src/game/test/test_utils.cc
  src/shared/objects/context/context.cc
  src/shared/utils/eventmanager/context_stack.cc
  src/shared/utils/utils.cc
  src/shared/utils/parser/expression_parser.cc
  src/shared/utils/fuzzy_search/fuzzy.cc
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(websocketpp CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/game)
add_executable(txtad.o ${GAME_SRC_FILES})
add_executable(txtad_test.o ${GAME_TEST_FILES})

add_compile_definitions(COMPILE_SPI_COMMANDS_FOR_CXX=1)
add_compile_definitions(JSON_DIAGNOSTICS=1)
if (COMPILE_FOR_SERVER)
  message("compiling for server...")
  target_compile_definitions(txtad.o PUBLIC _COMPILE_FOR_SERVER_)
else()
  message("compiling for localhost...")
endif()

target_link_libraries(txtad.o PRIVATE
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  httplib::httplib 
  websocketpp::websocketpp 
  Catch2::Catch2
)

target_link_libraries(txtad_test.o PRIVATE
  nlohmann_json::nlohmann_json 
  spdlog::spdlog 
  httplib::httplib 
  websocketpp::websocketpp 
  Catch2::Catch2
)


target_include_directories(txtad.o PUBLIC "src")
target_include_directories(txtad_test.o PUBLIC "src")
