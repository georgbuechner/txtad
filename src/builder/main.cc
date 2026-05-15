#include "builder/server/server.h"
#include "builder/utils/defines.h"
#include "builder/utils/defines.h"
#include "shared/utils/utils.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <thread>

using namespace std::chrono_literals;

int main() {
  util::SetUpLogger(builder::LoggerPath(), builder::LOGGER, spdlog::level::debug);
  util::LoggerContext scope(builder::LOGGER);

  nlohmann::json builder_config;
  if (auto json = util::LoadJsonFromDisc(builder::CONFIG)) {
    builder_config = *json;
  } else {
    std::runtime_error("No 'builder.config' file found!");
  }

  Builder builder(4081, "localhost", 4080, builder_config);

  std::thread thread_game_srv([&builder]() {
      builder.GameServerCommunication();
  });

  std::thread thread_http([&builder]() {
      builder.Start();
  });

  thread_game_srv.join();
  thread_http.join();
}
