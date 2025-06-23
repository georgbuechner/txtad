#include "shared/utils/parser/game_file_parser.h"
#include "game/utils/defines.h"
#include "shared/utils/eventmanager/listener.h"
#include "shared/utils/utils.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

void parser::LoadGameFiles(const std::string& path, std::map<std::string, std::shared_ptr<Context>>& contexts, 
    std::map<std::string, std::shared_ptr<Text>>& texts) {

  std::map<std::string, nlohmann::json> listeners;
  LoadObjects(path, contexts, texts, listeners);
  LoadListeners(contexts, std::move(listeners));
}

void parser::LoadObjects(const std::string& path, std::map<std::string, std::shared_ptr<Context>>& contexts, 
    std::map<std::string, std::shared_ptr<Text>>& texts, std::map<std::string, nlohmann::json>& listeners) {
  const std::string game_files_path = path + "/" + txtad::GAME_FILES;
  const size_t id_path_offset = game_files_path.length();

  // Create Contexts and Texts
  for (const auto& dir_entry : fs::recursive_directory_iterator(game_files_path)) {
    // If entry is file, create context:
    if (!dir_entry.is_directory()) {
      // Skip templates
      if (dir_entry.path().extension().string() == txtad::TEMPLATE_EXTENSION)
        continue;

      util::Logger()->debug("Game::Game. Parsing: {}", dir_entry.path().string());

      // Add new context
      if (dir_entry.path().extension() == txtad::CONTEXT_EXTENSION) {
        if (auto ctx = parser::CreateContextFromPath(dir_entry.path(), id_path_offset)) {
          if (contexts.count(ctx->id()) > 0) {
            util::Logger()->warn("Game::Game. Context \"{}\" already exists. Dublicate id?", ctx->id());
          } else {
            contexts.emplace(ctx->id(), ctx);
            // Potentially adds listener: 
            if (auto ctx_listeners = GetContextListener(dir_entry.path())) {
              listeners.emplace(ctx->id(), *ctx_listeners);
            }
            util::Logger()->debug("Game::Game. Created context: \"{}\".", ctx->id());
          }
        }
      }
      // Add new Text 
      else if (dir_entry.path().extension() == txtad::TEXT_EXTENSION) {
        if (auto txt = parser::CreateTextFromPath(dir_entry.path(), id_path_offset)) {
          if (texts.count(txt->id()) > 0) {
            util::Logger()->warn("Game::Game. Text \"{}\" already exists. Dublicate id?", txt->id());
          } else {
            texts[txt->id()] = txt;
            util::Logger()->debug("Game::Game. Created text: \"{}\".", txt->id());
          }
        }
      } else {
        util::Logger()->warn("Game::Game. File \"{}\" has invalid extension: \"{}\".", 
            dir_entry.path().string(), dir_entry.path().extension().string());
      }
    }
  }
}

void parser::LoadListeners(std::map<std::string, std::shared_ptr<Context>>& contexts,
    std::map<std::string, nlohmann::json> listeners) {
  for (const auto& [ctx_id, json_listeners] : listeners) {
    for (auto& json_listener : json_listeners.get<std::vector<nlohmann::json>>()) {
      // Replace "this"-field with ctx_id
      for (const auto* field : {"logic", "arguments"}) {
        if (json_listener.contains(field))
          json_listener[field] = util::ReplaceAll(json_listener[field], txtad::THIS_REPLACEMENT, ctx_id);
      }

      // Add/ Create context-listener
      if (json_listener.contains("ctx")) {
        std::string target_ctx_id = json_listener["ctx"];
        auto it_ctx = contexts.find((target_ctx_id == txtad::THIS_REPLACEMENT) ? ctx_id : target_ctx_id);
        if (it_ctx != contexts.end()) {
          contexts.at(ctx_id)->AddListener(std::make_shared<LContextForwarder>(json_listener, it_ctx->second));
        } else {
          util::Logger()->warn("parser::LoadListeners. For listener \"{}\", context \"{}\" not found.", 
              json_listener["id"].get<std::string>(), ctx_id);
        }
      } 
      // Add/ Create "normal" listener
      else {
        contexts.at(ctx_id)->AddListener(std::make_shared<LForwarder>(json_listener));
      }
    }
  }
}

std::shared_ptr<Context> parser::CreateContextFromPath(std::filesystem::path path, size_t id_path_offset) {
  if (const auto& json = util::LoadJsonFromDisc(path.string())) {
    // std::string base_id = path.parent_path().string().substr(id_path_offset);
    path.replace_extension();
    std::string base_id = path.string().substr(id_path_offset);
    return std::make_shared<Context>(base_id, *json);
  } 
  return nullptr;
}

std::shared_ptr<Text> parser::CreateTextFromPath(std::filesystem::path path, size_t id_path_offset) {
  if (const auto& json = util::LoadJsonFromDisc(path.string())) {
    path.replace_extension();
    std::string id = path.string().substr(id_path_offset);
    return std::make_shared<Text>(id, *json);
  }
  return nullptr;
}

std::optional<nlohmann::json> parser::GetContextListener(const std::filesystem::path& path) {
  if (const auto& json = util::LoadJsonFromDisc(path.string())) {
    if (json->contains("listeners")) {
      return std::optional(json->at("listeners"));
    }
  }
  return std::nullopt;
}
