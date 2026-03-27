#ifndef SRC_BUILDER_CREATOR_MANAGER_H
#define SRC_BUILDER_CREATOR_MANAGER_H 

#include "builder/creator/creator.h"
#include "builder/utils/defines.h"
#include "builder/utils/http_helpers.h"
#include "shared/utils/utils.h"
#include <catch2/internal/catch_clara.hpp>
#include <fmt/core.h>
#include <httplib.h>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>


class CreatorManager { 

  public:
    CreatorManager() {
      for (const auto& dir : std::filesystem::directory_iterator(builder::FILES_PATH + builder::CREATORS_PATH)) {
        if (dir.path().filename() == ".keep") {
          continue;
        }
        if (auto json = util::LoadJsonFromDisc(dir.path())) {
          std::shared_ptr<Creator> creator = std::make_shared<Creator>(*json);
          _creators[creator->username()] = creator;
        } else {
          util::Logger()->warn(fmt::format("CreatorManager: Invalid user-json at: ", dir.path().string()));
        }
      }
    }

    class AccessGuard {
      public: 
        AccessGuard(std::shared_ptr<Creator> instance) 
        : _instance(instance) {
          instance->lock();
        } 

        ~AccessGuard() {
          _instance->unlock();
        }

        std::shared_ptr<Creator> operator->() { 
          return _instance; 
        } 

      private: 
        std::shared_ptr<Creator> _instance; 
    };

    // methods 
    AccessGuard CreatorFromCookie(const httplib::Request& req) {
      std::string cookie = req.get_header_value("cookie");
      std::unique_lock ul(_mtx);
      if (_cookies.contains(GetSessionIdFromCookie(cookie))) {
        std::string username = _cookies.at(GetSessionIdFromCookie(cookie));
        if (_creators.contains(username) && _creators.at(username)) {
          return AccessGuard(_creators.at(username));
        }
      }      
      throw _http::_t_exception({404, "Username not found or invalid cookie"});
    }

    std::optional<AccessGuard> CreatorFromUsername(const std::string& username) {
      std::unique_lock ul(_mtx);
      if (_creators.count(username) > 0) {
        return AccessGuard(_creators.at(username));
      } else {
        return std::nullopt;
      }
    }

    std::string Register(const httplib::Request& req, httplib::Response& resp) {
      if (req.get_param_value_count("username") == 0 || req.get_param_value_count("password") == 0 
          || req.get_param_value_count("password_repeat") == 0 ) {
        resp.status = 401; 
        return "Invalid JSON or missing fields.";
      }
      const std::string username = req.get_param_value("username");
      const std::string password = req.get_param_value("password");
      const std::string password_repeat = req.get_param_value("password_repeat");

      if (auto creator = CreatorFromUsername(username)) {
        resp.status = 401; 
        return "Creator name already exists";
      } else if (password != password_repeat) {
        resp.status = 401; 
        return "Passwords don't match";
      } else if (!CheckPasswordStrength(password)) {
        resp.status = 401; 
        return "Password strength insuficient: " + password;
      } else {
        std::shared_ptr<Creator> creator = std::make_shared<Creator>(username, util::HashSha3512(password));
        std::unique_lock ul(_mtx);
        _creators[creator->username()] = creator;
        util::Logger()->info(fmt::format("Manager::Register. successfully created: {}", creator->username()));
        ul.unlock();
        resp.set_header("Set-Cookie", GenerateCookie(creator->username()).c_str());
        resp.status = 200; 
        return "";
      }
    }

    std::string Login(const httplib::Request& req, httplib::Response& resp) {
      if (req.get_param_value_count("username") == 0 || req.get_param_value_count("password") == 0) {
        resp.status = 401; 
        return "Invalid JSON or missing fields.";
      } else {
        const std::string username = req.get_param_value("username");
        const std::string password = util::HashSha3512(req.get_param_value("password"));

        if (auto creator = CreatorFromUsername(username)) {
          if ((*creator)->password() == password) {
            _mtx.unlock();
            resp.set_header("Set-Cookie", GenerateCookie((*creator)->username()).c_str());
            resp.status = 200; 
          } else {
            resp.status = 401; 
            return "Password incorrect";
          }
        } else {
          resp.status = 404; 
          return "Username not found.";
        }
      } 
      return "";
    }

    void Logout(const httplib::Request& req, httplib::Response& resp) {
      try {
        auto creator = CreatorFromCookie(req);
        _cookies.erase(GetSessionIdFromCookie(req.get_header_value("cookie")));
        resp.status = 200; 
        resp.set_redirect("/login", 303);
      } catch (_http::_t_exception) {
        resp.set_redirect(_http::Referer(req) + "?msg=Logout failed: Invalid cookie or username not found!", 303);
      }
    }

    std::string GetUserForGame(const std::string& game_id) {
      std::unique_lock ul(_mtx);
      for (const auto& it : _creators) {
        if (it.second->OwnsGame(game_id)) 
          return it.first;
      }
      return "";
    }

  private: 
    std::mutex _mtx;
    std::map<std::string, std::shared_ptr<Creator>> _creators;
    std::map<std::string, std::string> _cookies;

   /**
     * Creates random 32 characters to generates cookie. And maps cookie and given
     * user.
     * @param[in] username (username which is mapped on cookie)
     * @return returns cookie as string.
     */
    std::string GenerateCookie(const std::string& username) {
      // Get session-id.
      std::string sessid = util::CreateRandomString(32);
      // Add cookie for user.
      std::unique_lock ul(_mtx);
      _cookies[sessid] = username; 
      return builder::COOKIE_NAME + "=" + sessid + "; Path=/; SameSite=Strict";
    }

    std::string GetSessionIdFromCookie(std::string cookie) const {
      // Parse user-cookie-id from cookie and check if this user exists.
      size_t start = cookie.find(builder::COOKIE_NAME + "=") + builder::COOKIE_NAME_LEN + 1;
      size_t end = cookie.find(";");
      if (cookie.find(builder::COOKIE_NAME + "=") == std::string::npos) {
        return "";
      }
      if (end == std::string::npos)
        end = cookie.length();
      std::string session_id = cookie.substr(start, end-start);
      return session_id;
    }

    
    /**
     * Checks password strength.
     * Either 15 characters long, or 8 characters + 1 lowercase + 1 digit.
     * @param[in] password (given password to check)
     * @return whether strength is sufficient.
     */
    bool CheckPasswordStrength(const std::string& password) const {
      if (password.length() >= 15) return true;
      if (password.length() < 8) return false;
      bool digit = false, letter = false;
      for (size_t i=0; i<password.length(); i++) {
        if (std::isdigit(password[i]) != 0) {
          digit = true;
        } else if (std::islower(password[i]) != 0) {
          letter = true;
        } if (letter == true && digit == true) {
          return true;
        }
      }
      return false;
    }
};

#endif
