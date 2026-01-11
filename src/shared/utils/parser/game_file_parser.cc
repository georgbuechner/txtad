#include "builder/utils/defines.h"
#include "game/game/game.h"
#include "game/utils/defines.h"
#include "shared/objects/text/text.h"
#include "shared/objects/tests/test_case.h"
#include "shared/utils/eventmanager/listener.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::map<std::string, std::shared_ptr<Game>> parser::InitGames(const std::string& path) {
  std::map<std::string, std::shared_ptr<Game>> games;
  for (const auto& dir : std::filesystem::directory_iterator(path)) {
    const std::string filename = dir.path().filename();
    if (filename.front() == '.')
      continue;
    games[filename] = std::make_shared<Game>(dir.path(), filename);
    util::Logger()->info("MAIN:InitGames: Created game *{}* @{}", filename, dir.path().string());
  }
  return games;
}

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
  LoadObjects(path, contexts, texts, listeners);
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
    for (const auto& json_listener : json_listeners.get<std::vector<nlohmann::json>>()) {
      auto new_listener = CreateListenerFromJson(json_listener, ctx_id, contexts);
      // Add new listener
      if (new_listener) {
        contexts.at(ctx_id)->AddListener(new_listener);
        // Add to listeners for which function must be updated 
        if (json_listener.value("exec", false))
          exec_forwarders.push_back(new_listener);
      }
    }
  }
  return exec_forwarders;
}

std::shared_ptr<Listener> parser::CreateListenerFromJson(const nlohmann::json& og_json_listener, 
    const std::string& ctx_id, const std::map<std::string, std::shared_ptr<Context>>& contexts) {
  nlohmann::json json_listener = og_json_listener;
  // Replace "this"-field with ctx_id
  for (const auto* field : {"logic", "arguments"}) {
    if (json_listener.contains(field))
      json_listener[field] = util::ReplaceAll(util::ReplaceAll(json_listener[field], "_.", ctx_id + "."), 
          "_->", ctx_id + "->");
  }

  // Create context-listener
  if (json_listener.contains("ctx")) {
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
  return std::make_shared<LForwarder>(json_listener, og_json_listener);
}

std::shared_ptr<Context> parser::CreateContextFromPath(std::filesystem::path path, size_t id_path_offset) {
  if (const auto& json = util::LoadJsonFromDisc(path.string())) {
    // std::string base_id = path.parent_path().string().substr(id_path_offset);
    path.replace_extension();
    std::string base_id = path.string().substr(id_path_offset);
    if (json->at("description").is_object())
      return std::make_shared<Context>(base_id, *json, Text(json->at("description").get<nlohmann::json>()));
    else 
      return std::make_shared<Context>(base_id, *json, Text(json->at("description").get<std::string>()));
  } 
  return nullptr;
}

std::shared_ptr<Text> parser::CreateTextFromPath(std::filesystem::path path, size_t id_path_offset, std::string& txt_id) {
  if (const auto& json = util::LoadJsonFromDisc(path.string())) {
    path.replace_extension();
    txt_id = path.string().substr(id_path_offset);
    return std::make_shared<Text>(*json);
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

std::vector<TestCase> parser::LoadTestCases(const std::string& game_id) {
  util::Logger()->info("parser:LoadTestCases: path {}", txtad::GAMES_PATH + game_id + "/" + txtad::GAME_TESTS);
  std::vector<TestCase> test_cases;
  if (auto j_test_cases = util::LoadJsonFromDisc(txtad::GAMES_PATH + game_id + "/" + txtad::GAME_TESTS)) {
    for (const auto& test_case : j_test_cases->get<std::vector<nlohmann::json>>()) {
      test_cases.push_back(TestCase(test_case));
    }
  }
  return test_cases;
}
