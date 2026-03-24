#ifndef SRC_GAME_BUILDER_GAME_H
#define SRC_GAME_BUILDER_GAME_H 

#include "game/game/game.h"
#include "shared/utils/git_wrapper/git_wrapper.h"
#include <set>

class BuilderGame : public Game {
  public: 
    /**
     * @param[in] path 
     * @param[in] name
     */
    BuilderGame(std::string path, std::string name);

    // getter
    const std::set<std::string>& media_files() const;
    const std::set<std::string>& pending_media_files() const;
    const std::vector<std::string>& modified() const;
    const std::map<std::string, std::vector<git::CommitInfo>>& backup_infos();
    
    // setter
    void set_settings(txtad::Settings&& settings);

    // Medthods
    void StoreGame(std::string path = "");

    /**
     * Builder settings: Updates game description
     */
    void UpdateGameDescription(const std::string& description);

    /**
     * Builder settings: Adds path to hide from collaborators
     */
    void AddHiddenPath(const std::string& path);
    void RemoveHiddenPath(const std::string& path);

    void ResetModified(); 

    void AddModified(std::string mod);

    void UpdateBackupInfos();

    /**
     * Creates new context
     * @param[in] id
     */ 
    void CreateCtx(const std::string& id);

    /**
     * Creates new text
     * @param[in] id
     */ 
    void CreateTxt(const std::string& id);

    /**
     * Creates new directory
     * @param[in] id
     */ 
    void CreateDir(const std::string& id);

    void AddMediaFile(const std::string& id);
    void RemovePendingMediaFromDisc(const std::set<std::string>& pending_media_files);

    /**
     * Creates/Updates/Replaces listener while game is running
     */
    void CreateListenerInPlace(const std::string& listener_id, const nlohmann::json& json_listener, 
        const std::string& ctx_id, bool added);
    void RemoveListener(const std::string& listener_id, const std::string& ctx_id);
    void RemoveContext(const std::string& ctx_id);
    void RemoveText(const std::string& ctx_id);
    void RemoveMedia(const std::string& media_id);
    void RemoveDirectory(const std::string& path);

    /**
     * Updates text entry at given path.
     * If `txt` is nullptr, entry will be removed.
     */
    void UpdateText(std::string path, std::shared_ptr<Text> txt);

    /**
     * Tries to find all references to given context. 
     * - settings (inital events and contexts)
     * - listeners (arguments, logic)
     * - ctx-listeners 
     * - texts (text, events, logic)
     * @param[in] ctx_id
     * @return list of references
     */
    std::vector<std::string> GetCtxReferences(const std::string& ctx_id, const std::string& sub="") const;

    /**
     * Tries to find all references to given text. 
     * - settings (inital events)
     * - listeners (arguments)
     * - texts (text, events)
     * @param[in] text_id
     * @return list of references
     */
    std::vector<std::string> GetTextReferences(const std::string& text_id, const std::string& sub="") const;
    std::vector<std::string> GetMediaReferences(const std::string& text_id) const;

    /**
     * Gets paths and sub paths to all contexts and texts including type-info
     * (CTX, TXT, DIR)
     */
    std::map<std::string, builder::FileType> GetPaths() const;

    /**
     * Gets alls sub-paths to given path of all contexts and texts including type-info
     * (CTX, TXT, DIR)
     * @param path
     */
    std::map<std::string, builder::FileType> GetPaths(const std::string& path) const;

  private:
    std::set<std::string> _pending_directories;
    std::set<std::string> _pending_media_files;
    std::set<std::string> _deleted_media_files;
    std::set<std::string> _media_files;
    std::vector<std::string> _modified;
    std::map<std::string, std::vector<git::CommitInfo>> _backup_infos;

    static void AddRefsFoundInEvents(std::vector<std::string>& refs, const std::string& id, const std::string& events, 
        std::string&& msg);
    static void AddRefsFoundInString(std::vector<std::string>& refs, const std::string& id, const std::string& events, 
        const std::string& msg);
    static void AddRefsFoundInDesc(std::vector<std::string>& refs, const std::string& id, std::shared_ptr<Text> desc, 
        const std::string& ctx_id, bool media=false);
    void AddRefsFoundInText(std::vector<std::string>& refs, const std::string& id, bool media=false, const std::string& sub="") const;
};

#endif
