#ifndef SHARED_UTILS_PARSER_GAME_FILE_PARSER_H_
#define SHARED_UTILS_PARSER_GAME_FILE_PARSER_H_

#include "builder/utils/defines.h"
#include "shared/objects/context/context.h"
#include "shared/utils/eventmanager/listener.h"
#include <cstddef>
#include <filesystem>
#include <memory>

class Listener;
class Text;

namespace parser {
  using ExecListeners = std::vector<std::shared_ptr<Listener>>;

  std::map<std::string, builder::FileType> GetPaths(const std::string& game_path);
  std::map<std::string, builder::FileType> GetPaths(const std::string& game_path, const std::string path);

  template <class G>
  std::map<std::string, std::shared_ptr<G>> InitGames(const std::string& path) {
    std::map<std::string, std::shared_ptr<G>> games;
    for (const auto& dir : std::filesystem::directory_iterator(path)) {
      const std::string filename = dir.path().filename();
      if (filename.front() == '.')
        continue;
      games[filename] = std::make_shared<G>(dir.path(), filename);
      util::Logger()->info("MAIN:InitGames: Created game *{}* @{}", filename, dir.path().string());
    }
    return games;
  }

  ExecListeners LoadGameFiles(const std::string& path, 
      std::map<std::string, std::shared_ptr<Context>>& contexts, 
      std::map<std::string, std::shared_ptr<Text>>& texts);
  void LoadObjects(const std::string& path, std::map<std::string, std::shared_ptr<Context>>& contexts, 
      std::map<std::string, std::shared_ptr<Text>>& texts, std::map<std::string, nlohmann::json>& listeners);
  ExecListeners LoadListeners(std::map<std::string, std::shared_ptr<Context>>& contexts,
      const std::map<std::string, nlohmann::json>& listeners);
  std::shared_ptr<Listener> CreateListenerFromJson(const nlohmann::json& j_listener, const std::string& ctx_id,
      const std::map<std::string, std::shared_ptr<Context>>& contexts);

  std::shared_ptr<Context> CreateContextFromPath(std::filesystem::path path, size_t id_path_offset);
  std::shared_ptr<Text> CreateTextFromPath(std::filesystem::path path, size_t id_path_offset, 
      std::string& txt_id);
  std::optional<nlohmann::json> GetContextListener(const std::filesystem::path& path);

  // Helper
  std::string DoThisReplacement(std::string original, std::string ctx_id);

  /**
   * Extracts types from IDs: 
   * f.e. [general, rooms/freedom, items/potions/healing] 
   * => ["*rooms", "*items", "*items/potions"]
   * @param[in] ids 
   * @return types: path elements of IDs with prefixed "*"
   */
  std::vector<std::string> GetTypesFromIDs(const std::vector<std::string>& ids);
}

#endif
