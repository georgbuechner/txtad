#include "game/game/game.h"
#include "game/game/user.h"
#include "game/utils/defines.h"
#include "shared/objects/context/context.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/parser/game_file_parser.h"
#include "shared/utils/utils.h"
#include <cinttypes>
#include <fmt/core.h>
#include <functional>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>

Game::MsgFn Game::_cout = nullptr;

Game::Game(std::string path, std::string name) : _path(path), _name(name), _cur_user(nullptr), 
    _parser(std::bind(&Game::t_substitue_fn, this, std::placeholders::_1)),
    _settings(*util::LoadJsonFromDisc(_path + "/" + txtad::GAME_SETTINGS)) {
  util::SetUpLogger(txtad::FILES_PATH, _name, spdlog::level::debug);
  util::LoggerContext scope(_name);

  // Create baisc handlers
  LForwarder::set_overwite_fn(std::bind(&Game::h_add_to_eventqueue, this, std::placeholders::_1, 
        std::placeholders::_2));

  // Setup mechanics-context
  _mechanics_ctx = std::make_shared<Context>("ctx_mechanic", 0, false);

  // Context commands 
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_CTX01", "#ctx remove (.*)", 
        std::bind(&Game::h_remove_ctx, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_CTX02", "#ctx add (.*)", 
        std::bind(&Game::h_add_ctx, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_CTX03", "#ctx replace (.*)", 
        std::bind(&Game::h_replace_ctx, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_CTX04", "#ctx name (.*)", 
        std::bind(&Game::h_set_ctx_name, this, std::placeholders::_1, std::placeholders::_2)));
  
  // Attribute commands
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_ATTS01", "#sa (.*)", 
        std::bind(&Game::h_set_attribute, this, std::placeholders::_1, std::placeholders::_2)));

  // List commands
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_LST01", "#lst atts (.*)", 
        std::bind(&Game::h_list_attributes, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_LST02", "#lst* atts (.*)", 
        std::bind(&Game::h_list_all_attributes, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_LST03", "#lst ctxs (.*)", 
        std::bind(&Game::h_list_linked_contexts, this, std::placeholders::_1, std::placeholders::_2)));

  // Print commands
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_PRINT01", "#> (.*)", 
        std::bind(&Game::h_print, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_PRINT011", "#>> (.*)", 
        std::bind(&Game::h_print_no_prompt, this, std::placeholders::_1, std::placeholders::_2)));
   _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_PRINT02", "#-> (.*)", 
         std::bind(&Game::h_print_to, this, std::placeholders::_1, std::placeholders::_2)));

  // Reset commands
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_RESET01", "#reset game", 
        std::bind(&Game::h_reset_game, this, std::placeholders::_1, std::placeholders::_2)));
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H_RESET02", "#reset user", 
        std::bind(&Game::h_reset_user, this, std::placeholders::_1, std::placeholders::_2)));

  // Others
  _mechanics_ctx->AddListener(std::make_shared<LHandler>("H01", "#remove_user (.*)", 
        std::bind(&Game::h_remove_user, this, std::placeholders::_1, std::placeholders::_2)));

  auto exec_listeners = parser::LoadGameFiles(_path, _contexts, _texts);
  for (auto it : exec_listeners) {
    it->set_fn(std::bind(&Game::h_exec, this, std::placeholders::_1, std::placeholders::_2));
  }
}

Game::~Game() {
  spdlog::drop(_name);
}

// getter 
std::string Game::path() const { return _path; } 
std::string Game::name() const { return _name; } 
const std::map<std::string, std::shared_ptr<Context>>& Game::contexts() const { return _contexts; }
const std::map<std::string, std::shared_ptr<Text>>& Game::texts() const { return _texts; }
const Settings& Game::settings() const { return _settings; }
const std::shared_ptr<User>& Game::cur_user() { return _cur_user; }

// setter
void Game::set_msg_fn(Game::MsgFn fn) { _cout = fn; }

// methods 
void Game::HandleEvent(const std::string& user_id, const std::string& event) {
  util::LoggerContext scope(_name);

  util::Logger()->debug("Game::HandleEvent: Handling inp: {}", event);
  std::unique_lock ul(_mutex);

  // Inform all users about new connection
  if (event == "#new_connection") {
    for (const auto& it : _users) {
      it.second->HandleEvent(event + " " + user_id, _parser);
    }
  }

  // If user did not exist yet, create new user
  if (_users.count(user_id) == 0) {
    util::Logger()->debug("Game::HandleEvent: Creating new user: {}", user_id);
    // Create new user
    _cur_user = CreateNewUser(user_id);
    _cur_user->HandleEvent(_settings.initial_events(), _parser);
  } 
  // Otherwise, handle incomming event
  else {
    // _cur_user = _users.at(user_id);
    // _cur_user->HandleEvent(event, _parser);

    std::cout << "Checking valid user" << std::endl;
    if (auto cur_user = _users.at(user_id)) {
      std::cout << "Checking valid user success" << std::endl;
      _cur_user = cur_user;
      _cur_user->HandleEvent(event, _parser);
    } else {
      std::cout << "Checking valid user failed." << std::endl;
      util::Logger()->error("Game::HandleEvent. Invalid Game state. Existing user no longer valid! id: {}", 
        user_id);
    }
  }
  if (_cur_user) 
    _cout(_cur_user->id(), "#remove_prompt");
}

std::shared_ptr<User> Game::CreateNewUser(std::string user_id) {
  auto new_user = std::make_shared<User>(_name, user_id, [&_cout = _cout, user_id](const std::string& msg) {
    _cout(user_id, msg);
  }, _contexts, _texts, _settings.initial_ctx_ids());
  // Link base Context
  util::Logger()->debug("Link base Context.");
  new_user->LinkContextToStack(_mechanics_ctx);
  // Set new user
  util::Logger()->debug("Set new user");
  _users[user_id] = new_user;
  return new_user;
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

void Game::h_set_ctx_name(const std::string& event, const std::string& args) {
  util::Logger()->info("Game::h_set_ctx_name. args: {}", args);
  static const std::regex pattern("(.*) = (.*)");
  std::smatch base_match;
  if (std::regex_match(args, base_match, pattern)) {
    if (base_match.size() == 3) {
      if (auto ctx = _cur_user->GetContext(base_match[1].str())) {
        ctx->set_name(base_match[2].str());
      }
    }
  }
}

void Game::h_set_attribute(const std::string& event, const std::string& args) {
  util::Logger()->info("Game::h_set_attribute: {}", args);
  static std::vector<std::string> opts = {"+=", "-=", "++", "--", "*=", "/=", "="}; 
  // Find operator
  std::string opt;
  int pos = std::string::npos;
  for (const auto& it : opts) {
    auto p = args.find(it);
    if (p != std::string::npos) {
      opt = it;
      pos = p;
      break;
    }
  }
  if (pos == std::string::npos) {
    util::Logger()->warn("Game::h_set_attribute: no operator found: {}", args);
    return;
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
  if (_cur_user)
    _cur_user->AddToEventQueue(args);
  else 
    util::Logger()->error("Game::h_add_to_eventqueue. Invalid Game state. Current user no longer valid!");
}

void Game::h_exec(const std::string& event, const std::string& args) {
  const std::string event_queue = _cur_user->event_queue(); // Store event-queue
  _cur_user->HandleEvent(args, _parser);
  _cur_user->AddToEventQueue(event_queue); // And re-add here, since "HandleEvent" resets queue.
}

void Game::h_print(const std::string& event, const std::string& args) {
  util::Logger()->info("Handler::h_print: {} {}", event, args);
  std::string txt = GetText(event, args);
  _cout(_cur_user->id(), txt);
}

void Game::h_print_no_prompt(const std::string& event, const std::string& args) {
  util::Logger()->info("Handler::h_print: {} {}", event, args);
  std::string txt = GetText(event, args);
  _cout(_cur_user->id(), "$ " + txt);
}


void Game::h_print_to(const std::string& event, const std::string& args) {
  util::Logger()->info("Handler::h_print_to: {}, {}", event, args);
  std::string args_copy = args;
  if (args.front() == '*') {
    std::string txt = GetText(event, args.substr(1));
    util::Logger()->info("Handler::h_print_to: sending to all users: {}", txt);
    for (const auto& it : _users) {
      _cout(it.first, txt);
    }
  } else if (args.front() == '{') {
    util::Logger()->info("Handler::h_print_to: sending to user via subsitute");
    int closing = util::ClosingBracket(args, 1, '{', '}');
    if (closing != -1) {
      std::string subsitute = args.substr(1, closing-1);
      _cout(t_substitue_fn(subsitute), GetText(event, args_copy.substr(subsitute.length()+2)));
    } else {
      util::Logger()->warn("Handler::h_print_to: sending to user via subsitute: closing bracket not found.");
    }
  } else if (auto user_id = util::GetUserId(args_copy)) {
    util::Logger()->info("Handler::h_print_to: sending to user via ID");
    _cout(*user_id, GetText(event.substr(1), args_copy));
  } else {
    util::Logger()->warn("Handler::h_print_to. User-id or all-qualifier not found in {}", args);
  }
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
    std::vector<std::string> hidden;
    for (const auto& [key, value] : ctx->attributes()) {
      std::string str = "- " + key + ": " + value;
      if (key.front() == '_') 
        hidden.push_back(str);
      else
        _cout(_cur_user->id(), str);
    }
    for (const auto& it : hidden) {
      _cout(_cur_user->id(), it);
    }
  }
}

void Game::h_list_linked_contexts(const std::string& event, const std::string& args) {
  if (auto print_ctx = User::GetCtxPrint(args)) {
    if (print_ctx->_kind == txtad::CtxPrint::VARIABLE) {
      if (auto ctx = _cur_user->GetContext(print_ctx->_ctx_id)) {
        if (auto print_ctx_2 = User::GetCtxPrint(print_ctx->_what)) {
          if (print_ctx_2->_ctx_id == "*")
            _cout(_cur_user->id(), "linked contexts:");
          else
            _cout(_cur_user->id(), print_ctx_2->_ctx_id.substr(1) + ":");
          for (const auto& it : ctx->LinkedContexts(print_ctx_2->_ctx_id.substr(1))) {
            if (auto linked_ctx = it.lock()) {
              std::string str = "";
              if (print_ctx_2->_kind == txtad::CtxPrint::VARIABLE)
                User::AddVariableToText(linked_ctx, print_ctx_2->_what, str);
              else if (print_ctx_2->_kind == txtad::CtxPrint::ATTRIBUTE) {
                if (auto attr = linked_ctx->GetAttribute(print_ctx_2->_what))
                  str = *attr;
              }
              _cout(_cur_user->id(), "- " + str);
            }
          }
        }
      }
    }
  }
}

void Game::h_reset_game(const std::string& event, const std::string& ctx_id) {
  // Clear all contexts and texts
  _contexts.clear();
  _texts.clear();
  // Gather user IDs
  std::string cur_user_id = _cur_user->id();
  std::vector<std::string> user_ids; 
  for (const auto& it : _users) {
    user_ids.push_back(it.first);
  }
  // Reset all users
  for (const auto& it : user_ids) {
    _users.erase(it);
  }
  // Reload game files
  parser::LoadGameFiles(_path, _contexts, _texts);
  // Recreate all user
  for (const auto& it : user_ids) {
    CreateNewUser(it);
  }
  // Send clear all users and handle initial_events for all users
  for (const auto& it : _users) {
    _cout(it.first, txtad::CLEAR_CONSOLE);
    _cur_user = it.second;
    it.second->HandleEvent(_settings.initial_events(), _parser);
  }
  // Set current user to last current user
  _cur_user = _users.at(cur_user_id);
}

void Game::h_reset_user(const std::string& event, const std::string& ctx_id) {
  std::string user_id = _cur_user->id();
  _users.erase(user_id);
  _cur_user = CreateNewUser(user_id);
  _cout(user_id, txtad::CLEAR_CONSOLE);
  _cur_user->HandleEvent(_settings.initial_events(), _parser);
}

void Game::h_remove_user(const std::string& event, const std::string& ctx_id) {
  std::string user_id = _cur_user->id();
  _users.erase(user_id);
  _cur_user = nullptr;
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

std::string Game::GetText(std::string event, std::string args) {
  static const std::regex pattern(txtad::RE_PRINT_CTX);
  std::string txt = "";
  for (int i=0; i<args.length(); i++) {
    if (args[i] == '{') {
      int closing = util::ClosingBracket(args, i+1, '{', '}');
      if (closing != -1) {
        std::string subsitute = args.substr(i+1, closing-(i+1));
        util::Logger()->info("Handler::cout: found subsitute {}", subsitute);
        if (subsitute == txtad::EVENT_REPLACEMENT) {
          txt += event;
        } else if (subsitute == txtad::UID_REPLACEMENT) {
          txt += _cur_user->id();
        } else if (auto print_ctx = User::GetCtxPrint(subsitute)) {
          if (print_ctx->_kind == txtad::CtxPrint::VARIABLE)
            txt += GetText("", _cur_user->PrintCtx(print_ctx->_ctx_id, print_ctx->_what));
          else if (print_ctx->_kind == txtad::CtxPrint::ATTRIBUTE)
            txt += _cur_user->PrintCtxAttribute(print_ctx->_ctx_id, print_ctx->_what);
        } else if (_cur_user->texts().count(subsitute) > 0) {
          txt += GetText("", _cur_user->PrintTxt(subsitute));
        } else {
          util::Logger()->info("Handler::cout. {} did not match pattern.", subsitute);
        }
        i = closing;
      } else {
        txt += args[i];
      }
    } else {
      txt += args[i];
    }
  }
  return txt;
}
