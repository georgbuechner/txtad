#include "game/game/game.h"
#include "game/game/user.h"
#include "game/utils/defines.h"
#include "shared/objects/context/context.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <fmt/core.h>
#include <functional>
#include <memory>
#include <mutex>

Game::MsgFn Game::_cout = nullptr;

Game::Game(std::string path, std::string name) : _path(path), _name(name), _cur_user(nullptr), 
    _parser(std::bind(&Game::t_substitue_fn, this, std::placeholders::_1)),
    _settings(*util::LoadJsonFromDisc(_path + "/" + txtad::GAME_SETTINGS)) {
  util::SetUpLogger(txtad::FILES_PATH, _name, spdlog::level::debug);
  util::LoggerContext scope(_name);

  // Create baisc handlers
  LForwarder::set_overwite_fn(std::bind(&Game::h_add_to_eventqueue, this, std::placeholders::_1, std::placeholders::_2));

  // Setup mechanics-context
  _mechanics_ctx = std::make_shared<Context>("ctx_mechanic", 0, false);
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H1", "#ctx remove (.*)", 
        std::bind(&Game::h_remove_ctx, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H2", "#ctx add (.*)", 
        std::bind(&Game::h_add_ctx, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H3", "#ctx replace (.*)", 
        std::bind(&Game::h_replace_ctx, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H4", "#sa (.*)", 
        std::bind(&Game::h_set_attribute, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H5", "#la (.*)", 
        std::bind(&Game::h_list_attributes, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H6", "#laa (.*)", 
        std::bind(&Game::h_list_all_attributes, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H7", "#> (.*)", 
        std::bind(&Game::h_print, this, std::placeholders::_1, std::placeholders::_2)));

  parser::LoadGameFiles(_path, _contexts, _texts);
}

// getter 
std::string Game::path() const { return _path; } 
std::string Game::name() const { return _name; } 
const std::map<std::string, std::shared_ptr<Context>>& Game::contexts() const { return _contexts; }
const std::map<std::string, std::shared_ptr<Text>>& Game::texts() const { return _texts; }
const Settings& Game::settings() const { return _settings; }

// setter
void Game::set_msg_fn(Game::MsgFn fn) { _cout = fn; }

// methods 
void Game::HandleEvent(const std::string& user_id, const std::string& event) {
  util::LoggerContext scope(_name);

  util::Logger()->debug("Game::HandleEvent: Handling inp: {}", event);
  std::unique_lock ul(_mutex);

  // If user did not exist yet, create new user
  if (_users.count(user_id) == 0) {
    util::Logger()->debug("Game::HandleEvent: Creating new user: {}", user_id);
    // Create new user
    auto new_user = std::make_shared<User>(_name, user_id, [&_cout = _cout, user_id](const std::string& msg) {
      _cout(user_id, msg);
    }, _contexts, _texts, _settings.initial_ctx_ids());
    // Link base Context
    util::Logger()->debug("Link base Context.");
    new_user->LinkContextToStack(_mechanics_ctx);
    // Set new user
    util::Logger()->debug("Set new user");
    _users[user_id] = new_user;
    _cur_user = new_user;
    _cur_user->HandleEvent(_settings.initial_events(), _parser);
  } 
  // Otherwise, handle incomming event
  else {
    _cur_user = _users.at(user_id);
    _cur_user->HandleEvent(event, _parser);
  }
}

// handlers

void Game::h_add_ctx(const std::string& event, const std::string& ctx_id) {
  util::Logger()->info("Adding context: {}", ctx_id);
  if (_cur_user->contexts().count(ctx_id) > 0)
    _cur_user->LinkContextToStack(_cur_user->contexts().at(ctx_id));
  else 
    util::Logger()->warn("Handler::link_ctx. Context \"{}\" not found.", ctx_id);
}

void Game::h_remove_ctx(const std::string& event, const std::string& ctx_id) {
  util::Logger()->info("Removing context: {}", ctx_id);
  _cur_user->RemoveContext(ctx_id);
}

void Game::h_replace_ctx(const std::string& event, const std::string& args) {
  static const std::regex pattern("(.*) -> (.*)");
  std::smatch base_match;
  if (std::regex_match(args, base_match, pattern)) {
    if (base_match.size() == 3) {
      h_remove_ctx("", base_match[1].str());
      h_add_ctx("", base_match[2].str());
    }
  }
}

void Game::h_set_attribute(const std::string& event, const std::string& args) {
  util::Logger()->info("Game::h_set_attribute: {}", args);
  static std::vector<std::string> opts = {"+=", "-=", "++", "--", "*=", "/=", "="}; 
  // Find operator
  std::string opt;
  int pos = -1;
  for (const auto& it : opts) {
    auto p = args.find(it);
    if (p != std::string::npos) {
      opt = it;
      pos = p;
      break;
    }
  }
  // Split input into attribute and expression used to set attribute
  std::string attribute = args.substr(0, pos);
  std::string expression = args.substr(pos+opt.length()); 

  util::Logger()->info("attribute: {}, opt: {}, expression: {}", attribute, opt, expression);

  // Split attribute into ctx-id and attribute-id
  pos = attribute.rfind(".");
  if (pos == std::string::npos) {
    util::Logger()->warn("Game::h_set_attribute. Invalid attribute id! {}", attribute);
    return;
  }
  std::string ctx_id = attribute.substr(0, pos);
  std::string attribute_id = attribute.substr(pos+1);

  // Find context: 
  if (auto ctx = _cur_user->GetContext(ctx_id)) {
    if (auto attr = ctx->GetAttribute(attribute_id)) {
      std::string res = "";
      if (opt == "=")
        res = _parser.Evaluate(expression);
      else if (opt == "++")
        res = std::to_string(std::stoi(*attr) + 1);
      else if (opt == "--")
        res = std::to_string(std::stoi(*attr) - 1);
      else if (opt == "+=")
        res = std::to_string(std::stoi(*attr) + std::stoi(_parser.Evaluate(expression)));
      else if (opt == "-=")
        res = std::to_string(std::stoi(*attr) - std::stoi(_parser.Evaluate(expression)));
      else if (opt == "*=")
        res = std::to_string(std::stoi(*attr) * std::stoi(_parser.Evaluate(expression)));
      else if (opt == "/=")
        res = std::to_string(std::stoi(*attr) / std::stoi(_parser.Evaluate(expression)));
      util::Logger()->debug("User::PrintCtxAttribute: setting {} to {}", attribute_id, res);
      if (!ctx->SetAttribute(attribute_id, res))
        util::Logger()->warn("User::PrintCtxAttribute: Failed steting attribute: {} to {}", attribute_id, res);
    } else {
      util::Logger()->warn("User::PrintCtxAttribute: attribute {} not found in ctx {}", attribute_id, ctx->id());
    }
  } else {
    util::Logger()->warn("User::PrintCtxAttribute: ctx {} not found", ctx_id);
  }
}

void Game::h_add_to_eventqueue(const std::string& event, const std::string& args) {
  _cur_user->AddToEventQueue(args);
}

void Game::h_print(const std::string& event, const std::string& args) {
  util::Logger()->info("Handler::h_print: {} {}", event, args);
  static const std::regex pattern(R"(^(txt)\.(.*)$|^(ctx)\.(.*)(->(name|desc|description|attributes|all_attributes)|\.(.*))$)");
  std::string txt = "";
  for (int i=0; i<args.length();i++) {
    if (args[i] == '{') {
      int closing = util::ClosingBracket(args, i+1, '{', '}');
      if (closing != -1) {
        std::string subsitute = args.substr(i+1, closing-(i+1));
        util::Logger()->info("Handler::cout: found subsitute {}", subsitute);
        if (subsitute == "#event") {
          txt += event;
        } else {
          std::smatch match;
          if (std::regex_match(subsitute, match, pattern)) {
            if (match[1].matched)
              txt += _cur_user->PrintTxt(match[2].str());
            else if (match[4].matched && match[5].str().front() == '-') {
              txt += _cur_user->PrintCtx(match[4].str(), match[6].str());
            }
            else if (match[4].matched && match[5].str().front() == '.') {
              txt += _cur_user->PrintCtxAttribute(match[4].str(), match[7].str());
            }
          } else {
            util::Logger()->info("Handler::cout. {} did not match pattern.", subsitute);
          }
        }
        i = closing;
      } else {
        txt += args[i];
      }
    } else {
      txt += args[i];
    }
  }
  _cout(_cur_user->id(), txt);
}

void Game::h_list_attributes(const std::string& event, const std::string& ctx_id) {
  util::Logger()->info("Handler::h_list_attributes: {} {}", event, ctx_id);
  if (const auto& ctx = _cur_user->GetContext(ctx_id)) {
    _cout(_cur_user->id(), "Attributes:");
    for (const auto& [key, value] : ctx->attributes()) {
      if (key.front() != '_') 
        _cout(_cur_user->id(), "- " + key + ": " + value);
    }
  }
}

void Game::h_list_all_attributes(const std::string& event, const std::string& ctx_id) {
  util::Logger()->info("Handler::h_list_all_attributes: {} {}", event, ctx_id);
  if (const auto& ctx = _cur_user->GetContext(ctx_id)) {
    _cout(_cur_user->id(), "Attributes:");
    for (const auto& [key, value] : ctx->attributes()) {
      _cout(_cur_user->id(), "- " + key + ": " + value);
    }
  }
}

std::string Game::t_substitue_fn(const std::string& str) {
  std::vector<std::string> parts = {};
  // Check context attributes
  if ((parts = util::Split(str, ".")).size() == 2) {
    std::string ctx_id = parts[0];
    std::string attribute_key = parts[1];
    util::Logger()->debug("Game::t_substitue_fn. {}, {}", ctx_id, attribute_key);
    if (_cur_user->contexts().count(ctx_id) > 0) {
      if (auto attribute_value = _cur_user->contexts().at(ctx_id)->GetAttribute(attribute_key))
        return *attribute_value;
      else 
        util::Logger()->warn("Game::t_substitue_fn. In CTX {} attribute {} not found!", ctx_id, attribute_key);
    } else {
      util::Logger()->warn("Game::t_substitue_fn. CTX {} not found!", ctx_id);
    }
  }
  // Check context fields
  if ((parts = util::Split(str, "->")).size() == 2) {
    std::string ctx_id = parts[0];
    std::string field = parts[1];
    if (_cur_user->contexts().count(ctx_id) > 0) {
      if (field == "name")
        return _cur_user->contexts().at(ctx_id)->name();
      else
        util::Logger()->warn("Game::t_substitue_fn. FIELD {} does not exist in contexts!", field);
    } else {
      util::Logger()->warn("Game::t_substitue_fn. CTX {} not found!", ctx_id);
    }
  }
  return "";
}
