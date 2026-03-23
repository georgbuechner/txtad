#include "builder/game/builder_game.h"
#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>
#include "builder/utils/defines.h"
#include "game/utils/defines.h"
#include "shared/objects/context/context.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include "shared/utils/git_wrapper/git_wrapper.h"

namespace fs = std::filesystem;

BuilderGame::BuilderGame(std::string path, std::string name) : Game(path, name), _modified(false) {
  // If not yet initialized, initialize and create initial commit.
  if (!git::is_initialized(path)) {
    git::commit_on_save(path, {}, "initial_commit", "fux");
  }
  UpdateBackupInfos();

  // Load information on media files 
  parser::LoadMediaFileInformation(path, _media_files);
}

// getter
const std::set<std::string>& BuilderGame::media_files() const { return _media_files; }
const std::set<std::string>& BuilderGame::pending_media_files() const { return _pending_media_files; }
const std::vector<std::string>& BuilderGame::modified() const { return _modified; }
const std::map<std::string, std::vector<git::CommitInfo>>& BuilderGame::backup_infos() { return _backup_infos; }

// setter 
void BuilderGame::set_settings(txtad::Settings&& settings) { _settings = std::move(settings); }

// methods

void BuilderGame::ResetModified() {
  _modified.clear();
}

void BuilderGame::AddModified(std::string mod) {
  _modified.push_back(mod);
}

void BuilderGame::UpdateBackupInfos() {
  _backup_infos.clear();
  for (const auto& it : git::get_all_branches(_path)) {
    _backup_infos[it] = git::get_commits_of_branch(_path, it);
  }
}

void BuilderGame::CreateCtx(const std::string& id) {
  if (_contexts.contains(id)) {
    throw std::invalid_argument("Context already exists: " + id);
  }
  _contexts[id] = std::make_shared<Context>(id, 0);
  AddModified(fmt::format("Created new TXT: {}", id));
}

void BuilderGame::CreateTxt(const std::string& id) {
  if (_texts.contains(id)) {
    throw std::invalid_argument("Text already exists: " + id);
  }
  _texts[id] = std::make_shared<Text>(std::string(""));
  AddModified(fmt::format("Created new TXT: {}", id));
}

void BuilderGame::CreateDir(const std::string& id) {
  if (_pending_directories.contains(id)) {
    throw std::invalid_argument("Directory already exists: " + id);
  }
  _pending_directories.emplace(id);
}

void BuilderGame::AddMediaFile(const std::string& id) {
  if (_media_files.contains(id)) {
    util::Logger()->info("Overwriting existing media file");
  }
  _media_files.emplace(id);
  _pending_media_files.emplace(id);
  AddModified(fmt::format("Created new MEDIA: {}", id));
}

void BuilderGame::RemovePendingMediaFromDisc(const std::set<std::string>& pending_media_files) {
  for (const auto& it : pending_media_files) {
    std::filesystem::remove(_path + "/" + txtad::GAME_FILES + it);  // remove file only
  }
}

void BuilderGame::StoreGame(std::string path) {
  path = (path != "") ? path : _path;
  util::WriteJsonToDisc(path + "/" + txtad::GAME_SETTINGS, _settings.ToJson());
  util::WriteJsonToDisc(path + "/" + txtad::BUILDER_EXTENSION, _builder_settings.ToJson());
  
  // Remove all files that are not media-files
  const std::string path_prefix = path + "/" + txtad::GAME_FILES;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(path_prefix)) {
    if (!entry.is_regular_file()) continue;  // only files

    // Make sure to never remove media-files 
    auto relative = std::filesystem::relative(entry.path(), path_prefix).string(); 
    if (!_media_files.contains(relative)) {
      std::filesystem::remove(entry.path());  
    }
  }
  // Remove deleted media files: 
  RemovePendingMediaFromDisc(_deleted_media_files);
  util::RemoveEmptyDirs(path_prefix);

  // Save current state
  for (const auto& ctx : _contexts) {
    fs::path p(path + "/" + txtad::GAME_FILES + ctx.first + txtad::CONTEXT_EXTENSION);
    fs::create_directories(p.parent_path());
    util::WriteJsonToDisc(p.string(), ctx.second->json());
  }
  for (const auto& txt : _texts) {
    fs::path p(path + "/" + txtad::GAME_FILES + txt.first + txtad::TEXT_EXTENSION);
    fs::create_directories(p.parent_path());
    util::WriteJsonToDisc(p.string(), txt.second->json());
  }
  for (const auto& dir : _pending_directories) {
    fs::path p(path + "/" + txtad::GAME_FILES + dir);
    fs::create_directories(p.parent_path());
  }
}

void BuilderGame::UpdateGameDescription(const std::string& description) {
  _builder_settings._description = description;
}

void BuilderGame::AddHiddenPath(const std::string& path) {
  _builder_settings._hidded_dirs.insert(path);
  AddModified(fmt::format("Added hidden path: {}", path));
}

void BuilderGame::RemoveHiddenPath(const std::string& path) {
  if (_builder_settings._hidded_dirs.contains(path)) {
    _builder_settings._hidded_dirs.erase(path);
    AddModified(fmt::format("Removed hidden path: {}", path));
  } else {
    util::Logger()->warn("BuilderGame::RemoveHiddenPath. Path \"{}\" not found in hidden paths", path);
  }
}

void BuilderGame::CreateListenerInPlace(const std::string& listener_id, const nlohmann::json& json_listener, 
        const std::string& ctx_id, bool added) {
  if (!_contexts.contains(ctx_id)) {
    throw std::invalid_argument("Game::CreateListenerInPlace: context " + ctx_id + " not found!");
  }
  // Create listener from json
  auto new_listener = parser::CreateListenerFromJson(json_listener, ctx_id, _contexts);
  // If contains exec (direct execution), set exec function
  if (json_listener.contains("exec")) {
    new_listener->set_fn(std::bind(&BuilderGame::h_exec, this, std::placeholders::_1, std::placeholders::_2));
  }
  _contexts.at(ctx_id)->AddListener(new_listener);
  if (listener_id != new_listener->id()) {
    _contexts.at(ctx_id)->RemoveListener(listener_id);
    AddModified(fmt::format("Removed listener {} for ctx: {}", listener_id, ctx_id));
    AddModified(fmt::format("Added listener {} for ctx: {}", new_listener->id(), ctx_id));
  } else if (added) {
    AddModified(fmt::format("Added listener {} for ctx: {}", new_listener->id(), ctx_id));
  } else {
    AddModified(fmt::format("Updated listener {} for ctx: {}", listener_id, ctx_id));
  }
}

void BuilderGame::RemoveListener(const std::string& listener_id, const std::string& ctx_id) {
  if (!_contexts.contains(ctx_id)) {
    throw std::invalid_argument("Game::CreateListenerInPlace: context " + ctx_id + " not found!");
  }
  _contexts.at(ctx_id)->RemoveListener(listener_id);
}

void BuilderGame::RemoveContext(const std::string& ctx_id) {
  if (!_contexts.contains(ctx_id)) {
    throw std::invalid_argument("Game::RemoveContext: context " + ctx_id + " not found!");
  }
  _contexts.erase(ctx_id);
  AddModified(fmt::format("Removed ctx {}.", ctx_id));
}

void BuilderGame::RemoveText(const std::string& txt_id) {
  if (!_texts.contains(txt_id)) {
    throw std::invalid_argument("Game::RemoveText: text " + txt_id + " not found!");
  }
  _texts.erase(txt_id);
  AddModified(fmt::format("Removed txt {}.", txt_id));
}

void BuilderGame::RemoveMedia(const std::string& media_id) {
  if (!_media_files.contains(media_id)) {
    throw std::invalid_argument("Game::RemoveMedia: media " + media_id + " not found!");
  }
  // Remove from media-files (for display) 
  _media_files.erase(media_id);
  // Also remove from pending (to avoid creating when saving game-state)
  if (_pending_media_files.contains(media_id)) {
    _pending_media_files.erase(media_id);
  }
  // Add to deleted files (to remove file when saving game-state)
  _deleted_media_files.emplace(media_id);
  AddModified(fmt::format("Removed media {}.", media_id));
  // NOTE: File will eventually be deleted when storing game
}

void BuilderGame::RemoveDirectory(const std::string& path) {
  auto txt_ks = std::views::keys(_texts);
  for (const auto& it : std::vector<std::string>(txt_ks.begin(), txt_ks.end())) {
    if (it.find(path) == 0) {
      RemoveText(it);
    }
  }
  auto ctx_ks = std::views::keys(_contexts);
  for (const auto& it : std::vector<std::string>(ctx_ks.begin(), ctx_ks.end())) {
    if (it.find(path) == 0) {
      RemoveContext(it);
    }
  }
  std::vector<std::string> media_files_to_remove;
  for (const auto& it : _media_files) {
    if (it.find(path) == 0) {
      media_files_to_remove.push_back(it);
    }
  }
  for (const auto& it : media_files_to_remove) {
    RemoveMedia(it);
  }
  if (_pending_directories.contains(path)) {
    _pending_directories.erase(path);
  }
}

void BuilderGame::UpdateText(std::string path, std::shared_ptr<Text> txt) {
  if (txt) {
    _texts[path] = txt;
  } else if (_texts.contains(path)) {
    _texts.erase(path);
  } else {
    util::Logger()->warn("Game::UpdateText. Updated text {} empty but also did not exist.", path);
  }
}

std::vector<std::string> BuilderGame::GetCtxReferences(const std::string& ctx_id, const std::string& sub) const {
  std::vector<std::string> refs;
  
  // settings
  AddRefsFoundInEvents(refs, ctx_id, _settings.initial_events(), "SETTINGS -> initial_events: ");
  for (const auto& inital_ctx : _settings.initial_ctx_ids()) {
    if (inital_ctx == ctx_id) {
      refs.push_back(fmt::format("SETTINGS -> initial_contexts")); 
    }
  }

  // contexts
  for (const auto& [id, ctx] : _contexts) {
    // If sub defined ignore all internal - references
    if (!sub.empty() && id.find(sub) == 0) {
      continue;
    }

    // Description
    AddRefsFoundInDesc(refs, ctx_id, ctx->description(), id);

    // Listeners (linked ctx's, arguments and logic)
    for (const auto& [listener_id, listener] : ctx->listeners()) {
      try {
        if (listener->ctx_id() == ctx_id) {
          refs.push_back(fmt::format("CTX: {} -> Listener {} (linked-ctx)", id, listener_id)); 
        } 
      } catch (util::invalid_base_class_call&) {}
      AddRefsFoundInString(refs, ctx_id, listener->arguments(), fmt::format("CTX: {} -> Listener {} (arguments: {})", id, listener_id, "{}"));
      AddRefsFoundInString(refs, ctx_id, listener->logic(), fmt::format("CTX: {} -> Listener {} (logic: {})", id, listener_id, "{}"));
    }
  }
  
  // texts
  AddRefsFoundInText(refs, ctx_id, false, sub);
  return refs;
}

std::vector<std::string> BuilderGame::GetTextReferences(const std::string& text_id, const std::string& sub) const {
  std::vector<std::string> refs;
  
  // settings
  AddRefsFoundInEvents(refs, text_id, _settings.initial_events(), "SETTINGS -> initial_events: ");

  // contexts
  for (const auto& [id, ctx] : _contexts) {
    // If sub defined ignore all internal - references
    if (!sub.empty() && id.find(sub) == 0) {
      continue;
    }

    // Description
    AddRefsFoundInDesc(refs, text_id, ctx->description(), id);

    // Listeners (arguments)
    for (const auto& [listener_id, listener] : ctx->listeners()) {
      AddRefsFoundInString(refs, text_id, listener->arguments(), 
          fmt::format("CTX: {} -> Listener {} (arguments: {})", id, listener_id, "{}"));
    }
  }

  // texts
  AddRefsFoundInText(refs, text_id, false, sub);
  return refs;
}

std::vector<std::string> BuilderGame::GetMediaReferences(const std::string& media_id) const {
  std::vector<std::string> refs;
  util::Logger()->info("GetMediaReferences: {}", media_id);
  
  // settings
  AddRefsFoundInEvents(refs, media_id, _settings.initial_events(), "SETTINGS -> initial_events: ");

  // contexts
  for (const auto& [id, ctx] : _contexts) {
    // Description
    AddRefsFoundInDesc(refs, media_id, ctx->description(), id, true);
    // Listeners (arguments)
    for (const auto& [listener_id, listener] : ctx->listeners()) {
      AddRefsFoundInString(refs, media_id, listener->arguments(), 
          fmt::format("CTX: {} -> Listener {} (arguments: {})", id, listener_id, "{}"));
    }
  }

  // texts
  AddRefsFoundInText(refs, media_id, true);
  return refs;
}

void BuilderGame::AddRefsFoundInEvents(std::vector<std::string>& refs, const std::string& id, const std::string& events, 
    std::string&& msg) {
  for (const auto& event : util::Split(events, ";")) {
    if (event.find(id) != std::string::npos) {
      refs.push_back(msg + event);
    }
  }
}

void BuilderGame::AddRefsFoundInString(std::vector<std::string>& refs, const std::string& id, const std::string& str, 
    const std::string& msg) {
  if (str.find(id) != std::string::npos) {
    refs.push_back(std::vformat(msg, std::make_format_args(str)));
  }
}

void BuilderGame::AddRefsFoundInDesc(std::vector<std::string>& refs, const std::string& id, std::shared_ptr<Text> desc, 
    const std::string& ctx_id, bool media) {
  std::string search_for = (media) ? id : "{" + id;
  std::shared_ptr<Text> txt = desc;
  int index = 0;
  do {
    AddRefsFoundInString(refs, search_for, txt->txt(), fmt::format("CTX: {} -> Description[{}] (text): {}", ctx_id, index, "{}"));
    AddRefsFoundInEvents(refs, id, txt->permanent_events(), fmt::format("CTX: {} -> Description[{}] (permanent_events): ", ctx_id, index));
    AddRefsFoundInEvents(refs, id, txt->one_time_events(), fmt::format("CTX: {} -> Description[{}] (one_time_events): ", ctx_id, index));
    txt = txt->next();
    index++;
  } while(txt != nullptr);
}

void BuilderGame::AddRefsFoundInText(std::vector<std::string>& refs, const std::string& id, bool media, 
    const std::string& sub) const {
  std::string search_for = (media) ? id : "{" + id;
  // texts
  for (const auto& it : _texts) {
    // If sub defined ignore all internal - references
    if (!sub.empty() && it.first.find(sub) == 0) {
      continue;
    }
    std::shared_ptr<Text> txt = it.second;
    int index = 0;
    do {
      // Since text are refereced in brackets add "{" to ID.
      AddRefsFoundInString(refs, search_for, txt->txt(), fmt::format("TXT {}[{}] -> text: {}", it.first, index, "{}"));
      AddRefsFoundInEvents(refs, id, txt->permanent_events(), fmt::format("TXT {}[{}] -> permanent_events: ", it.first, index));
      AddRefsFoundInEvents(refs, id, txt->one_time_events(), fmt::format("TXT {}[{}] -> one_time_events: ", it.first, index));
      txt = txt->next();
      index++;
    } while(txt != nullptr);
  }
}

std::map<std::string, builder::FileType> BuilderGame::GetPaths() const {
  std::map<std::string, builder::FileType> paths;


  // Add DIRs first, so they get overwritten
  for (const auto& dir : _pending_directories) {
    paths.emplace(dir, builder::FileType::DIR);
    for (const auto& sub_dir : util::GetSubpaths(dir)) {
      paths.emplace(sub_dir, builder::FileType::DIR);
    }
  }

  // Add media files
  for (const auto& media : _media_files) {
    paths.emplace(media, builder::FileType::MEDIA);
    for (const auto& sub_path : util::GetSubpaths(media)) {
      paths.emplace(sub_path, builder::FileType::DIR);
    }
  }

  // TODO (fux): account for identical dir and context names?
  auto cs = std::views::keys(_contexts);
  for (const auto& dir : util::GetSubpaths((std::vector<std::string>){cs.begin(), cs.end()})) {
    // This implies that there cannot be a context and a (sub-)dir with the same name
    paths.emplace(dir, builder::FileType::DIR);
  }
  for (const auto& ctx_id : _contexts) {
    paths.emplace(ctx_id.first, builder::FileType::CTX);
  }

  // TODO (fux): account for identical context and text names?
  auto ts = std::views::keys(_texts);
  for (const auto& dir : util::GetSubpaths((std::vector<std::string>){ts.begin(), ts.end()})) {
    paths.emplace(dir, builder::FileType::DIR);
  }
  for (const auto& txt_id : _texts) {
    paths.emplace(txt_id.first, builder::FileType::TXT);
  }

  return paths;
}

std::map<std::string, builder::FileType> BuilderGame::GetPaths(const std::string& sub_path) const {
  std::map<std::string, builder::FileType> paths; 
  for (const auto& [path, type] : GetPaths()) {
    if (path.find(sub_path) == 0 && path.length() != sub_path.length()) {
      std::string name = (sub_path.empty()) ? path : path.substr(sub_path.length()+1);
      // We only want depth=1
      if (name.find("/") == std::string::npos)
        paths.emplace(name, type);
    }
  }
  return paths;
}
