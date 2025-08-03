#ifndef SHARED_UTILS_PARSER_GAME_FILE_PARSER_H_
#define SHARED_UTILS_PARSER_GAME_FILE_PARSER_H_

#include "shared/objects/context/context.h"
#include "shared/objects/text/text.h"
#include "shared/utils/eventmanager/listener.h"
#include <cstddef>
#include <filesystem>
#include <memory>

class Listener;

namespace parser {
  using ExecListeners = std::vector<std::shared_ptr<Listener>>;

  ExecListeners LoadGameFiles(const std::string& path, 
      std::map<std::string, std::shared_ptr<Context>>& contexts, 
      std::map<std::string, std::shared_ptr<Text>>& texts);
  void LoadObjects(const std::string& path, std::map<std::string, std::shared_ptr<Context>>& contexts, 
      std::map<std::string, std::shared_ptr<Text>>& texts, std::map<std::string, nlohmann::json>& listeners);
  ExecListeners LoadListeners(std::map<std::string, std::shared_ptr<Context>>& contexts,
    std::map<std::string, nlohmann::json> listeners);

  std::shared_ptr<Context> CreateContextFromPath(std::filesystem::path path, size_t id_path_offset);
  std::shared_ptr<Text> CreateTextFromPath(std::filesystem::path path, size_t id_path_offset, std::string& txt_id);
  std::optional<nlohmann::json> GetContextListener(const std::filesystem::path& path);
}

#endif
