/*
 * @author: fux
 */

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include "websocket_server.h"
#include "game/utils/defines.h"
#include "shared/utils/utils.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;


WebsocketServer::EventHandlerFn WebsocketServer::_handle_event = nullptr;

WebsocketServer::WebsocketServer() {}

WebsocketServer::~WebsocketServer() {
  _server.stop();
}

void WebsocketServer::set_handle_event(EventHandlerFn fn) {
  _handle_event = fn;
}


void WebsocketServer::Start(int port) {
  try { 
    util::Logger()->debug("WSS::Start: set_access_channels");
    _server.set_access_channels(websocketpp::log::alevel::none);
    _server.clear_access_channels(websocketpp::log::alevel::frame_payload);
    _server.init_asio(); 
    _server.set_message_handler(bind(&WebsocketServer::OnMessage, this, &_server, ::_1, ::_2)); 
    _server.set_open_handler(bind(&WebsocketServer::OnOpen, this, ::_1));
    _server.set_close_handler(bind(&WebsocketServer::OnClose, this, ::_1));
    _server.set_reuse_addr(true);
    _server.listen(port); 
    _server.start_accept(); 
    util::Logger()->info("WSS::Start: Successfully started websocket server on port: {}", port);
    _server.run(); 
  } catch (websocketpp::exception const& e) {
    util::Logger()->error("WSS::Start: websocketpp::exception: {}", e.what());
  } catch (std::exception& e) {
    util::Logger()->error("WSS::Start: websocketpp::exception: {}", e.what());
  }
}

void WebsocketServer::OnOpen(websocketpp::connection_hdl hdl) {
  std::unique_lock ul_connections(_mutex);
  const std::string user_id = ConnectionIDToString(hdl.lock().get());
  if (_connections.count(user_id) == 0) {
    _connections[user_id] = hdl;
  }
  else
    util::Logger()->warn("WSS::OnOpen: New connection, but connection already exists!");
}

void WebsocketServer::OnClose(websocketpp::connection_hdl hdl) {
  std::unique_lock ul_connections(_mutex);
  const std::string user_id = ConnectionIDToString(hdl.lock().get());
  if (_connections.count(user_id) > 0) {
    try {
      // Send remove user command to game
      const auto& game_id = _user_game_mapping.at(user_id);
      ul_connections.unlock();
      _handle_event(user_id, game_id, txtad::REMOVE_USER + " " + user_id);
      ul_connections.lock();
      // Delete connection.
      if (_connections.size() > 1)
        _connections.erase(user_id);
      else 
        _connections.clear();
      ul_connections.unlock();
      util::Logger()->debug("WSS::OnClose: Connection closed successfully.");
    } catch (std::exception& e) {
      util::Logger()->error("WSS::OnClose: Failed to close connection: {}", e.what());
    }
  }
  else 
    util::Logger()->warn("WSS::OnClose: Connection closed, but connection didn't exist!");
}


void WebsocketServer::OnMessage(t_server* srv, websocketpp::connection_hdl hdl, t_message_ptr msg) {
  util::Logger()->debug("WSS::OnMessage: Got new message: {}", msg->get_payload());
  try {
    nlohmann::json json = nlohmann::json::parse(msg->get_payload());
    if (json.contains("game") && json.contains("event")) {
      const auto& user_id = ConnectionIDToString(hdl.lock().get());
      const auto& game_id = json["game"];
      // if no entry exists, add new entry to user-game-mapping
      if (_user_game_mapping.count(user_id) == 0)
        _user_game_mapping[user_id] = game_id;
      _handle_event(user_id, game_id, json["event"]);
    } else {
      util::Logger()->warn("WSS::OnMessage: Missing \"game\" or \"event\"");
    }
  } catch (std::exception& e) {
    util::Logger()->warn("WSS::OnMessage: Unkown error: ", e.what());
  }
}

void WebsocketServer::SendMessage(const std::string& user_id, const std::string& payload) {
  std::shared_lock sl_connections(_mutex);
  if (_connections.count(user_id) > 0) {
    try {
    _server.send(_connections.at(user_id), payload, websocketpp::frame::opcode::value::text);
    } catch (std::exception& e) {
      util::Logger()->info("WSS::OnMessage: Failed to send message (connection dead?): ", e.what());
    }
  }
}

std::string const WebsocketServer::ConnectionIDToString(t_connection_id connection_id) {
  std::stringstream stream; 
  stream << connection_id;
  return stream.str();
}
