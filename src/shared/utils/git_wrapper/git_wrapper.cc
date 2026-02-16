
#include "shared/utils/utils.h"
#include "shared/utils/git_wrapper/git_wrapper.h"
#include <ctime>
#include <filesystem>
#include <git2/branch.h>
#include <git2/checkout.h>
#include <git2/commit.h>
#include <git2/deprecated.h>
#include <git2/diff.h>
#include <git2/errors.h>
#include <git2/index.h>
#include <git2/oid.h>
#include <git2/refs.h>
#include <git2/repository.h>
#include <git2/signature.h>
#include <git2/status.h>
#include <git2/tree.h>
#include <git2/types.h>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct LibGit2Global {
  LibGit2Global() { git_libgit2_init(); }
  ~LibGit2Global() { git_libgit2_shutdown(); }
};

static LibGit2Global libgit2_global;


static void throw_git(int err, const char* what) {
  if (err < 0) {
    const git_error* e = git_error_last(); 
    std::ostringstream out; 
    out << what << " (libgit2 err=" << err << ")";
    if (e && e->message) out << ": " << e->message; 
    throw std::runtime_error(out.str());
  }
}

static std::string build_commit_msg(const std::vector<std::string>& changes, 
    const std::optional<std::string>& msg) {
  std::ostringstream out; 
  out << util::iso_utc_now();

  // Add message if exists
  if (msg && !msg->empty()) out << " - " << *msg;
  out << "\n\n";

  if (!changes.empty()) {
    out << "Changes:\n";
    for (const auto& it : changes) {
      out << "- " << it << "\n";
    }
  } else {
    out << "Changes: Nothing changed.";
  }
  return out.str();
}

static bool has_head(git_repository* repo) {
  int err = git_repository_head_unborn(repo); 
  if (err == 1) return false; 
  if (err == 0) return true;
  throw_git(err, "git_repository_head_unborn");
  return false;
}

static bool branch_exists(git_repository* repo, const std::string& branch_name) {
  git_reference* ref = nullptr; 
  int err = git_branch_lookup(&ref, repo, branch_name.c_str(), GIT_BRANCH_LOCAL);
  if (err == 0) {
    git_reference_free(ref);
    return true;
  } else if (err == GIT_ENOTFOUND) {
    return false;
  }
  throw_git(err, "git_branch_lookup");
}

static bool index_has_changes(git_repository* repo, git_index* index) {
  if (!has_head(repo)) {
    return git_index_entrycount(index);
  }

  git_oid head_oid{};
  git_reference* head_ref = nullptr; 
  throw_git(git_repository_head(&head_ref, repo), "git_repository_head");

  git_object* head_obj = nullptr;
  int err = git_reference_peel(&head_obj, head_ref, GIT_OBJECT_COMMIT);
  git_reference_free(head_ref);
  throw_git(err, "git_reference_peel(HEAD)");

  git_commit* head_commit = (git_commit*)head_obj; 
  
  git_tree* head_tree = nullptr; 
  throw_git(git_commit_tree(&head_tree, head_commit), "git_commit_tree"); 

  git_tree* index_tree = nullptr; 
  throw_git(git_index_write_tree_to(&head_oid, index, repo), "git_index_write_tree_to"); 
  throw_git(git_tree_lookup(&index_tree, repo, &head_oid), "git_tree_lookup(index_tree)"); 

  git_diff* diff = nullptr; 
  throw_git(git_diff_tree_to_tree(&diff, repo, head_tree, index_tree, nullptr), "git_diff_tree_to_tree");

  bool changed = git_diff_num_deltas(diff) > 0;

  git_diff_free(diff);
  git_tree_free(index_tree);
  git_tree_free(head_tree);
  git_commit_free(head_commit);
  return changed;
}

static void stage_all(git_repository* repo, git_index** out_index) {
  git_index* index = nullptr; 
  throw_git(git_repository_index(&index, repo), "git_repository_index");

  // Add/Update everything that exists in the working tree
  git_strarray pathspec = {nullptr, 0}; 
  throw_git(git_index_add_all(index, &pathspec, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr), "git_index_add_all");

  // Remove from index anything that was deleted 
  git_status_options opts = GIT_STATUS_OPTIONS_INIT; 
  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS; 

  git_status_list* status = nullptr; 
  throw_git(git_status_list_new(&status, repo, &opts), "git_status_list_new"); 

  const size_t n = git_status_list_entrycount(status);
  for (size_t i=0; i<n; i++) {
    const git_status_entry* e = git_status_byindex(status, i);
    if (!e) {
      continue;
    } 

    // If path is deleted from working tree, remove it from index 
    if (e->status == GIT_STATUS_WT_DELETED) {
      const char* p = (e->index_to_workdir && e->index_to_workdir->old_file.path) 
        ? e->index_to_workdir->old_file.path : nullptr;
      if (p) {
        git_index_remove_bypath(index, p);
      }
    }
  }

  git_status_list_free(status); 
  throw_git(git_index_write(index), "git_index_write");
  *out_index = index;
}

static git_signature* make_signature(const std::string& name, const std::string& email) {
  git_signature* sig = nullptr; 

  std::time_t now = std::time(nullptr); 
  throw_git(git_signature_new(&sig, name.c_str(), email.c_str(), now, 0), "git_signature_new");
  
  return sig;
}

static void ensure_repo_initialized(const fs::path& repo_dir) {
  if (git::is_initialized(repo_dir)) return; 

  fs::create_directories(repo_dir);
  git_repository* repo = nullptr; 
  throw_git(git_repository_init(&repo, repo_dir.c_str(), 0), "git_repository_init");
  git_repository_free(repo);
}

static git_commit* head_commit(git_repository* repo) {
  git_reference* head_ref = nullptr; 
  throw_git(git_repository_head(&head_ref, repo), "git_repository_head");

  git_object* obj = nullptr; 
  throw_git(git_reference_peel(&obj, head_ref, GIT_OBJECT_COMMIT), "git_reference_peel");
  git_reference_free(head_ref);

  return (git_commit*)obj;
}

static std::string oid_to_hex(const git_oid* oid) {
  char buf[GIT_OID_HEXSZ + 1];
  git_oid_tostr(buf, sizeof(buf), oid);
  return std::string(buf);
}

static std::string timestamp_utc_compact() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&t, &tm);

    // e.g. 20260216-154233
    char buf[32];
    std::snprintf(buf, sizeof(buf),
                  "%04d%02d%02d-%02d%02d%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

static bool local_branch_exists(git_repository* repo, const std::string& branch_name) {
    git_reference* ref = nullptr;
    int err = git_branch_lookup(&ref, repo, branch_name.c_str(), GIT_BRANCH_LOCAL);
    if (err == 0) { git_reference_free(ref); return true; }
    if (err == GIT_ENOTFOUND) return false;
    throw_git(err, "git_branch_lookup");
    return false;
}

static std::string make_unique_restore_branch(git_repository* repo) {
    // Try restore/<timestamp>, then restore/<timestamp>-N
    const std::string base = "restore/" + timestamp_utc_compact();
    std::string candidate = base;

    int suffix = 1;
    while (local_branch_exists(repo, candidate)) {
        candidate = base + "-" + std::to_string(suffix++);
        if (suffix > 10000) {
            throw std::runtime_error("Failed to generate unique restore branch name.");
        }
    }

    // Validate refname according to Git rules
    const std::string full_ref = "refs/heads/" + candidate;
    if (!git_reference_is_valid_name(full_ref.c_str())) {
        throw std::runtime_error("Generated invalid restore branch name: " + candidate);
    }
    return candidate;
}

static git_commit* lookup_commit(git_repository* repo, const std::string& sha) {
    git_oid oid{};
    throw_git(git_oid_fromstr(&oid, sha.c_str()), "git_oid_fromstr");

    git_commit* commit = nullptr;
    throw_git(git_commit_lookup(&commit, repo, &oid), "git_commit_lookup");
    return commit; // caller frees
}


bool git::is_initialized(const std::filesystem::path& repo_dir) {
  return (fs::exists(repo_dir / ".git"));
}


void git::commit_on_save(const std::filesystem::path& repo_dir, const std::vector<std::string>& changes, 
      const std::optional<std::string>& msg, const std::string& author_name, const std::string& email) {
  ensure_repo_initialized(repo_dir);

  git_repository* repo = nullptr; 
  throw_git(git_repository_open(&repo, repo_dir.c_str()), "git_repository_open");

  git_index* index = nullptr; 
  stage_all(repo, &index); 

  // If nothing changed, do nothing 
  if (!index_has_changes(repo, index)) {
    git_index_free(index);
    git_repository_free(repo);
    return;
  }

  git_oid tree_oid{}; 
  throw_git(git_index_write_tree(&tree_oid, index), "git_index_write_tree");

  git_tree* tree = nullptr; 
  throw_git(git_tree_lookup(&tree, repo, &tree_oid), "git_tree_lookup");

  git_commit* parent_commit = nullptr; 
  if (has_head(repo)) {
    git_reference* head_ref = nullptr; 
    throw_git(git_repository_head(&head_ref, repo), "git_repository_head");

    git_object* head_obj = nullptr;
    throw_git(git_reference_peel(&head_obj, head_ref, GIT_OBJECT_COMMIT), "git_reference_peel(HEAD)");
    git_reference_free(head_ref);

    parent_commit = (git_commit*)head_obj;
  }

  git_signature* author = make_signature(author_name, email);
  git_signature* committer = make_signature(author_name, email);

  const std::string message = build_commit_msg(changes, msg);

  git_oid commit_oid{};
  if (parent_commit) {
    git_commit* parents[] = {parent_commit}; 
    throw_git(git_commit_create(&commit_oid, repo, "HEAD", author, committer, "UTF-8", message.c_str(), tree, 
          1, parents),  "git_commit_create");
  } else {
    throw_git(git_commit_create(&commit_oid, repo, "HEAD", author, committer, "UTF-8", message.c_str(), tree, 
          0, nullptr),  "git_commit_create");
  }

  // Cleanup 
  git_signature_free(author);
  git_signature_free(committer);
  if (parent_commit) git_commit_free(parent_commit);
  git_tree_free(tree);
  git_index_free(index);
  git_repository_free(repo);
}

void git::switch_to_new_branch(const std::filesystem::path& repo_dir, const std::string& branch_name) {
  git_repository* repo = nullptr; 
  throw_git(git_repository_open(&repo, repo_dir.c_str()), "git_repository_open");

  // Musst have a commit to branch from (should always exist!) 
  int unborn = git_repository_head_unborn(repo); 
  if (unborn < 0) {
    throw_git(unborn, "git_repository_head_unborn");
  } else if (unborn == 1) {
    git_repository_free(repo);
    throw std::runtime_error("Cannot create branch: repository has not commits yet!");
  }

  if (branch_exists(repo, branch_name)) {
    git_repository_free(repo);
    throw std::runtime_error("Branch already exists: " + branch_name);
  }

  git_commit* hc = head_commit(repo);

  git_reference* new_branch_ref = nullptr; 
  throw_git(git_branch_create(&new_branch_ref, repo, branch_name.c_str(), hc, 0), "git_branch_create");
  git_commit_free(hc);

  // Set HEAD to refs/head/<branch_name> 
  const char* refname = git_reference_name(new_branch_ref);
  throw_git(git_repository_set_head(repo, refname), "git_repository_set_head"); 

  git_reference_free(new_branch_ref);
  git_repository_free(repo);
}

std::vector<std::string> git::get_all_branches(const std::filesystem::path& repo_dir) {
  git_repository* repo = nullptr; 
  throw_git(git_repository_open(&repo, repo_dir.c_str()), "git_repository_open");
  
  git_branch_iterator* it = nullptr; 
  throw_git(git_branch_iterator_new(&it, repo, GIT_BRANCH_LOCAL), "git_branch_iterator_new");

  std::vector<std::string> out;
  git_reference* ref = nullptr; 
  git_branch_t type; 

  while (git_branch_next(&ref, &type, it) == 0) {
    const char* name = nullptr; 
    throw_git(git_branch_name(&name, ref), "git_brance_name");
    if (name) {
      out.emplace_back(name);
    }
    git_reference_free(ref); 
    ref == nullptr;
  }

  git_branch_iterator_free(it);
  git_repository_free(repo);
  return out;
}


std::vector<git::CommitInfo> git::get_commits_of_branch(const std::filesystem::path repo_dir,
      const std::string& branch_name, size_t limit) {
  git_repository* repo = nullptr;
  throw_git(git_repository_open(&repo, repo_dir.c_str()), "git_repository_open");

  git_reference* branch_ref = nullptr;
  int err = git_branch_lookup(&branch_ref, repo, branch_name.c_str(), GIT_BRANCH_LOCAL);
  if (err == GIT_ENOTFOUND) {
    git_repository_free(repo);
    throw std::runtime_error("Branch not found: " + branch_name);
  }
  throw_git(err, "git_branch_lookup");

  git_oid tip_oid{};
  const git_oid* target = git_reference_target(branch_ref);
  if (!target) {
    git_reference_free(branch_ref);
    git_repository_free(repo);
    throw std::runtime_error("Branch has no direct target (unexpected).");
  }
  tip_oid = *target;

  git_revwalk* walk = nullptr;
  throw_git(git_revwalk_new(&walk, repo), "git_revwalk_new");
  git_revwalk_sorting(walk, GIT_SORT_TIME | GIT_SORT_TOPOLOGICAL);
  throw_git(git_revwalk_push(walk, &tip_oid), "git_revwalk_push");

  std::vector<CommitInfo> out;
  out.reserve(std::min<size_t>(limit, 256));

  git_oid oid{};
  while (out.size() < limit && git_revwalk_next(&oid, walk) == 0) {
    git_commit* c = nullptr;
    throw_git(git_commit_lookup(&c, repo, &oid), "git_commit_lookup");

    CommitInfo ci;
    ci._oid = oid_to_hex(&oid);

    const char* summary = git_commit_summary(c);
    ci._summary = summary ? summary : "";

    const git_signature* a = git_commit_author(c);
    if (a && a->name) ci._author = a->name;

    ci._time_utc = static_cast<std::int64_t>(git_commit_time(c));

    out.push_back(std::move(ci));
    git_commit_free(c);
  }

  git_revwalk_free(walk);
  git_reference_free(branch_ref);
  git_repository_free(repo);
  return out;
}

std::string git::get_current_branch(const std::filesystem::path& repo_dir) {
  git_repository* repo = nullptr; 
  throw_git(git_repository_open(&repo, repo_dir.c_str()), "git_repository_open");

  git_reference* head = nullptr; 
  int err = git_repository_head(&head, repo);
  if (err == GIT_EUNBORNBRANCH) {
    git_repository_free(repo);
    throw std::runtime_error("Detached HEAD in repo: " + repo_dir.string() + " (admin must fix locally!)");
  }
  throw_git(err, "git_repository_head");

  std::string branch_name;
  if (git_reference_is_branch(head)) {
    // Normal case
    const char* name = nullptr;
    throw_git(git_branch_name(&name, head), "git_branch_name");
    if (name) {
      branch_name = name;
    } else {
      throw std::runtime_error("Invalid branch name in repo: " + repo_dir.string() + " (admin must fix locally!)");
    }
  } else {
    throw std::runtime_error("Detached HEAD in repo: " + repo_dir.string() + " (admin must fix locally!)");
  }
  git_reference_free(head);
  git_repository_free(repo);
  return branch_name;
}

// Restores working tree to commit_sha, creates and checks out restore/<timestamp> branch.
// Always FORCE + REMOVE_UNTRACKED.
// Returns created branch name.
std::string git::restore_to_commit_always_branch(const std::filesystem::path& repo_dir, const std::string& commit_sha) {
  git_repository* repo = nullptr;
  throw_git(git_repository_open(&repo, repo_dir.c_str()), "git_repository_open");

  git_commit* commit = lookup_commit(repo, commit_sha);

  // 1) Create a unique restore branch pointing to the commit
  const std::string branch_name = make_unique_restore_branch(repo);

  git_reference* new_branch_ref = nullptr;
  throw_git(git_branch_create(&new_branch_ref, repo, branch_name.c_str(), commit, /*force*/ 0), "git_branch_create(restore)");

  // 2) Set HEAD to that branch (never detached)
  const char* refname = git_reference_name(new_branch_ref); // refs/heads/restore/...
  throw_git(git_repository_set_head(repo, refname), "git_repository_set_head");

  // 3) Checkout the commit into the working tree, overwriting local changes,
  //    and removing untracked files.
  git_checkout_options co = GIT_CHECKOUT_OPTIONS_INIT;
  co.checkout_strategy = GIT_CHECKOUT_FORCE | GIT_CHECKOUT_REMOVE_UNTRACKED;

  // Checkout the tree from the target commit (same as branch tip)
  throw_git(git_checkout_tree(repo, (git_object*)commit, &co), "git_checkout_tree");

  // Also checkout HEAD to ensure index/WT align with the now-current branch ref
  // (Usually a no-op, but keeps things consistent.)
  throw_git(git_checkout_head(repo, &co), "git_checkout_head");

  // 4) Refresh index (good hygiene after destructive checkout)
  git_index* index = nullptr;
  throw_git(git_repository_index(&index, repo), "git_repository_index");
  throw_git(git_index_read(index, /*force*/ 1), "git_index_read");
  git_index_free(index);

  // Cleanup
  git_reference_free(new_branch_ref);
  git_commit_free(commit);
  git_repository_free(repo);

  return branch_name;
}
