#include "builder/server/server.h"
#include "builder/utils/defines.h"
#include "builder/utils/defines.h"
#include "shared/utils/utils.h"
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

using namespace std::chrono_literals;

int main() {

  util::SetUpLogger(builder::FILES_PATH, builder::LOGGER, spdlog::level::debug);
  util::LoggerContext scope(builder::LOGGER);

  Builder builder(4081, "localhost", 4080);

  std::thread thread_game_srv([&builder]() {
      builder.GameServerCommunication();
  });

  std::thread thread_http([&builder]() {
      builder.Start();
  });

  thread_game_srv.join();
  thread_http.join();
}
