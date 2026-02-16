#ifndef SRC_SHARED_UTILS_GIT_WRAPPER_H
#define SRC_SHARED_UTILS_GIT_WRAPPER_H

#include <filesystem>
#include <git2.h>
#include <optional>
#include <vector>

namespace git {
  struct CommitInfo {
    std::string _oid;
    std::string _summary; 
    std::string _author;
    std::int64_t _time_utc;
  };

  bool is_initialized(const std::filesystem::path& repo_dir); 

  void commit_on_save(const std::filesystem::path& repo_dir, const std::vector<std::string>& changes, 
      const std::optional<std::string>& msg, const std::string& author, 
      const std::string& email="@txtad-builder.local");

  void switch_to_new_branch(const std::filesystem::path& repo_dir, const std::string& branch_name);

  std::string get_current_branch(const std::filesystem::path& repo_dir);

  std::vector<std::string> get_all_branches(const std::filesystem::path& repo_dir);

  std::vector<CommitInfo> get_commits_of_branch(const std::filesystem::path repo_dir,
      const std::string& branch_name, size_t limit = 200);

  std::string restore_to_commit_always_branch(const std::filesystem::path& repo_dir, const std::string& commit_sha);
}

#endif
