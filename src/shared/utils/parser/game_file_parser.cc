#include "builder/utils/defines.h"
#include "game/utils/defines.h"
#include "shared/objects/text/text.h"
#include "shared/objects/tests/test_case.h"
#include "shared/utils/eventmanager/listener.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::map<std::string, builder::FileType> parser::GetPaths(const std::string& game_path) {
  const std::string game_files_path = game_path + "/" + txtad::GAME_FILES;
  const size_t id_path_offset = game_files_path.length();
  std::map<std::string, builder::FileType> paths;
  for (const auto& dir_entry : fs::recursive_directory_iterator(game_files_path)) {
    std::string p_name = dir_entry.path().string();
    if (id_path_offset < p_name.length())
      p_name = p_name.substr(id_path_offset);
    if (dir_entry.is_directory()) {
      paths.emplace(p_name, builder::FileType::DIR);
    } else if (dir_entry.path().extension() == txtad::CONTEXT_EXTENSION) {
      paths.emplace(fs::path(p_name).replace_extension(), builder::FileType::CTX);
    } else if (dir_entry.path().extension() == txtad::TEXT_EXTENSION) {
      paths.emplace(fs::path(p_name).replace_extension(), builder::FileType::TXT);
    } else if (dir_entry.path().extension() == txtad::TEMPLATE_EXTENSION) {
      paths.emplace(fs::path(p_name).replace_extension(), builder::FileType::TEM);
    }
  }
  return paths;
}

std::map<std::string, builder::FileType> parser::GetPaths(const std::string& game_path, const std::string path) {
  const std::string game_files_path = game_path + "/" + txtad::GAME_FILES;
  const size_t id_path_offset = game_files_path.length();
  std::map<std::string, builder::FileType> paths;
  for (const auto& dir_entry : fs::recursive_directory_iterator(game_files_path)) {
    std::string p_name = dir_entry.path().string();
    if (id_path_offset < p_name.length())
      p_name = p_name.substr(id_path_offset);
    if (path != "<use-all>" && ((path == "" && p_name.find("/") != std::string::npos) 
          || (path != "" && fs::path(p_name).remove_filename() != path + "/"))) {
      continue;
    }
    if (dir_entry.is_directory()) {
      paths.emplace(fs::path(p_name).filename(), builder::FileType::DIR);
    } else if (dir_entry.path().extension() == txtad::CONTEXT_EXTENSION) {
      paths.emplace(fs::path(p_name).filename().replace_extension(), builder::FileType::CTX);
    } else if (dir_entry.path().extension() == txtad::TEXT_EXTENSION) {
      paths.emplace(fs::path(p_name).filename().replace_extension(), builder::FileType::TXT);
    } else if (dir_entry.path().extension() == txtad::TEMPLATE_EXTENSION) {
      paths.emplace(fs::path(p_name).filename().replace_extension(), builder::FileType::TEM);
    }
  }
  return paths;
}

parser::ExecListeners parser::LoadGameFiles(const std::string& path, 
    std::map<std::string, std::shared_ptr<Context>>& contexts, 
    std::map<std::string, std::shared_ptr<Text>>& texts) {
  std::map<std::string, nlohmann::json> listeners;
  util::Logger()->debug("Loading game objects: " + path);
  LoadObjects(path, contexts, texts, listeners);
  util::Logger()->debug("Loading game listeners: " + path);
  return LoadListeners(contexts, std::move(listeners));
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
        std::string txt_id;
        if (auto txt = parser::CreateTextFromPath(dir_entry.path(), id_path_offset, txt_id)) {
          if (texts.count(txt_id) > 0) {
            util::Logger()->warn("Game::Game. Text \"{}\" already exists. Dublicate id?", txt_id);
          } else {
            texts[txt_id] = txt;
            util::Logger()->debug("Game::Game. Created text: \"{}\".", txt_id);
          }
        }
      } else {
        util::Logger()->warn("Game::Game. File \"{}\" has invalid extension: \"{}\".", 
            dir_entry.path().string(), dir_entry.path().extension().string());
      }
    }
  }
}

parser::ExecListeners parser::LoadListeners(std::map<std::string, std::shared_ptr<Context>>& contexts,
    const std::map<std::string, nlohmann::json>& listeners) {
  ExecListeners exec_forwarders;
  for (const auto& [ctx_id, json_listeners] : listeners) {
    try {
      for (const auto& json_listener : json_listeners.get<std::vector<nlohmann::json>>()) {
        try {
          auto new_listener = CreateListenerFromJson(json_listener, ctx_id, contexts);
          // Add new listener
          if (new_listener) {
            contexts.at(ctx_id)->AddListener(new_listener);
            // Add to listeners for which function must be updated 
            if (json_listener.value("exec", false))
              exec_forwarders.push_back(new_listener);
          }
        } catch (std::exception& e) {
          throw std::invalid_argument("Failed creating listener " + json_listener.value("id", 
                "no-listener-id") + " for " + ctx_id + ": " + e.what());
        }
      }
    } catch (std::exception& e) {
      throw std::invalid_argument("Failed creating listeners for ctx: " + ctx_id + ": " + e.what());
    }
  }
  return exec_forwarders;
}

std::shared_ptr<Listener> parser::CreateListenerFromJson(const nlohmann::json& og_json_listener, 
    const std::string& ctx_id, const std::map<std::string, std::shared_ptr<Context>>& contexts) {
  nlohmann::json json_listener = og_json_listener;
  util::Logger()->debug("creating listener: {}", json_listener.dump());

  // Replace "this"-field with ctx_id
  for (const auto* field : {"logic", "arguments"}) {
    if (json_listener.contains(field)) {
      json_listener[field] = DoThisReplacement(json_listener[field], ctx_id);
    }
  }

  // Create context-listener
  if (json_listener.contains("ctx") && !json_listener.at("ctx").get<std::string>().empty()) {
    util::Logger()->debug("CREATING CTX-FORWARDER");
    std::string target_ctx_id = json_listener["ctx"];
    auto it_ctx = contexts.find((target_ctx_id == "<_>") ? ctx_id : target_ctx_id);
    if (it_ctx != contexts.end()) {
      return std::make_shared<LContextForwarder>(json_listener, it_ctx->second, og_json_listener);
    } else {
      util::Logger()->warn("parser::LoadListeners. For listener \"{}\", context \"{}\" not found.", 
          json_listener["id"].get<std::string>(), ctx_id);
      return nullptr;
    }
  } 
  // Create "normal" listener
  util::Logger()->debug("CREATING FORWARDER");
  return std::make_shared<LForwarder>(json_listener, og_json_listener);
}

void parser::LoadMediaFileInformation(const std::string& path, std::set<std::string>& media_files) {
  const std::string game_files_path = path + "/" + txtad::GAME_FILES;
  for (const auto& dir_entry : fs::recursive_directory_iterator(game_files_path)) {
    if (!dir_entry.is_directory() && txtad::MEDIA_EXTENSTIONS.contains(dir_entry.path().extension().string())) {
      media_files.insert(dir_entry.path().string().substr(game_files_path.size()));
    }
  }
}

std::shared_ptr<Context> parser::CreateContextFromPath(std::filesystem::path path, size_t id_path_offset) {
  if (const auto& json = util::LoadJsonFromDisc(path.string())) {
    // std::string base_id = path.parent_path().string().substr(id_path_offset);
    path.replace_extension();
    std::string base_id = path.string().substr(id_path_offset);
    if (json->at("description").is_object() || json->at("description").is_array())
      return std::make_shared<Context>(base_id, *json, std::make_shared<Text>(json->at("description").get<nlohmann::json>()));
    else 
      return std::make_shared<Context>(base_id, *json, std::make_shared<Text>(json->at("description").get<std::string>()));
  } 
  return nullptr;
}

std::shared_ptr<Text> parser::CreateTextFromPath(std::filesystem::path path, size_t id_path_offset, std::string& txt_id) {
  if (const auto& json = util::LoadJsonFromDisc(path.string())) {
    path.replace_extension();
    txt_id = path.string().substr(id_path_offset);
    return std::make_shared<Text>(*json, "");
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

std::string parser::DoThisReplacement(std::string original, std::string ctx_id) {
  return util::ReplaceAll(util::ReplaceAll(original, "_.", ctx_id + "."), "_->", ctx_id + "->");
}

std::vector<std::string> parser::GetTypesFromIDs(const std::vector<std::string>& ids) {
  auto elements = util::GetSubpaths(ids);
  std::vector<std::string> vec = {"*"};
  std::transform(elements.begin(), elements.end(), std::back_inserter(vec), [](const auto& e) { return "*" + e; });
  return vec;
}
