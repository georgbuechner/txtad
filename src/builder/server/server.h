#ifndef BUILDER_SERVER_SERVER_H_
#define BUILDER_SERVER_SERVER_H_

#include "builder/creator/manager.h"
#include "builder/utils/jinja_helpers.h"
#include "builder/game/builder_game.h"
#include "httplib.h"
#include "shared/objects/tests/test_case.h"
#include <memory>

class Builder {
  public: 
    Builder(int port, std::string cli_address, int cli_port);

    // methods 
    void Start(); 
    void GameServerCommunication();
  
  private: 
    httplib::Server _srv;
    httplib::Client _cli;

    _jinja::Env _env;

    CreatorManager _manager;

    std::shared_mutex _mtx_games;
    std::map<std::string, std::shared_ptr<BuilderGame>> _games;

    // methods 
    void LoadTemplate(httplib::Response& resp, const std::string& page, 
        const jinja2::ValuesMap& params = jinja2::ValuesMap());
    
    jinja2::ValuesMap CreateParams(const httplib::Request& req, std::string game_id = "", 
        std::string type = "", const std::vector<TestCase>& test_cases = std::vector<TestCase>());

    // API 
    void ApiRegister(const httplib::Request& req, httplib::Response& resp);
    void ApiLogin(const httplib::Request& req, httplib::Response& resp);
    void ApiLogout(const httplib::Request& req, httplib::Response& resp);
    void ApiAddRequest(const httplib::Request& req, httplib::Response& resp);
    void ApiAcceptRequest(const httplib::Request& req, httplib::Response& resp);
    void ApiDenyRequest(const httplib::Request& req, httplib::Response& resp);
    void ApiReloadGame(const httplib::Request& req, httplib::Response& resp);
    void ApiStopGame(const httplib::Request& req, httplib::Response& resp);
    void ApiRunGame(const httplib::Request& req, httplib::Response& resp);

    ///< list of IDs from all contexts
    void ApiContextIDs(const httplib::Request& req, httplib::Response& resp);
    ///< list of IDs from all tests
    void ApiTextIDs(const httplib::Request& req, httplib::Response& resp);
    ///< list of IDs from all contexts and tests
    void ApiIDs(const httplib::Request& req, httplib::Response& resp);
    ///< list of Types (path elements) from contexts
    void ApiTypes(const httplib::Request& req, httplib::Response& resp);
    ///< list of Types (path elements) AND IDs from alls contexts 
    void ApiContextTypes(const httplib::Request& req, httplib::Response& resp);
    ///< map of contexts and their attributes 
    void ApiContextAttributes(const httplib::Request& req, httplib::Response& resp);
    ///< map of types and the attributes that belong to ALL contexts of this type
    void ApiTypesAttributes(const httplib::Request& req, httplib::Response& resp);

    void ApiCtxReferences(const httplib::Request& req, httplib::Response& resp);
    void ApiTxtReferences(const httplib::Request& req, httplib::Response& resp);
    void ApiDirReferences(const httplib::Request& req, httplib::Response& resp);

    // PAGES 
    void PagesGame(const httplib::Request& req, httplib::Response& resp);

    // SAVE ELEMENTS 
    void SaveSettings(const httplib::Request& req, httplib::Response& resp);
    void SaveTests(const httplib::Request& req, httplib::Response& resp);
    void SaveCtxMeta(const httplib::Request& req, httplib::Response& resp);
    void SaveAttribute(const httplib::Request& req, httplib::Response& resp);
    void SaveListener(const httplib::Request& req, httplib::Response& resp);
    void SaveDescriptionElement(const httplib::Request& req, httplib::Response& resp, bool add_new);
    void SaveTextElement(const httplib::Request& req, httplib::Response& resp, bool add_new);
    void SaveGame(const httplib::Request& req, httplib::Response& resp);
    
    // REMOVES ELEMENTS
    void RemoveAttribute(const httplib::Request& req, httplib::Response& resp);
    void RemoveListener(const httplib::Request& req, httplib::Response& resp);
    void RemoveDescriptionElement(const httplib::Request& req, httplib::Response& resp);
    void RemoveTextElement(const httplib::Request& req, httplib::Response& resp);

    void RemoveText(const httplib::Request& req, httplib::Response& resp);
    void RemoveContext(const httplib::Request& req, httplib::Response& resp);
    void RemoveDirectory(const httplib::Request& req, httplib::Response& resp);

    void RestoreGame(const httplib::Request& req, httplib::Response& resp);
    void RestoreArchive(const httplib::Request& req, httplib::Response& resp);

    // helper 
    std::shared_ptr<Text> CreateTextElement(const httplib::Request& req, std::shared_ptr<Text> text, 
        bool add_new);
    std::shared_ptr<Text> RemoveTextElement(const httplib::Request& req, std::shared_ptr<Text> text);
};

#endif
