/*
 * @author: fux
 */

#include "server/websocket_server.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;


WebsocketServer::WebsocketServer() {}

WebsocketServer::~WebsocketServer() {
  server_.stop();
}

void WebsocketServer::Start(int port) {
  try { 
    std::cout << "WebsocketFrame::Start: set_access_channels" << std::endl;
    server_.set_access_channels(websocketpp::log::alevel::none);
    server_.clear_access_channels(websocketpp::log::alevel::frame_payload);
    server_.init_asio(); 
    server_.set_message_handler(bind(&WebsocketServer::on_message, this, &server_, ::_1, ::_2)); 
    server_.set_open_handler(bind(&WebsocketServer::OnOpen, this, ::_1));
    server_.set_close_handler(bind(&WebsocketServer::OnClose, this, ::_1));
    server_.set_reuse_addr(true);
    server_.listen(port); 
    server_.start_accept(); 
    std::cout << "WebsocketFrame:: successfully started websocket server on port: " << port << std::endl;
    server_.run(); 
  } catch (websocketpp::exception const& e) {
    std::cout << "WebsocketFrame::Start: websocketpp::exception: " << e.what() << std::endl;
  } catch (std::exception& e) {
    std::cout << "WebsocketFrame::Start: websocketpp::exception: " << e.what() << std::endl;
  }
}

void WebsocketServer::OnOpen(websocketpp::connection_hdl hdl) {
  std::unique_lock ul_connections(mutex_connections_);
  if (connections_.count(hdl.lock().get()) == 0)
    connections_[hdl.lock().get()] = hdl;
  else
    std::cout << "WebsocketFrame::OnOpen: New connection, but connection already exists!" << std::endl;
}

void WebsocketServer::OnClose(websocketpp::connection_hdl hdl) {
  std::unique_lock ul_connections(mutex_connections_);
  if (connections_.count(hdl.lock().get()) > 0) {
    try {
      // Delete connection.
      if (connections_.size() > 1)
        connections_.erase(hdl.lock().get());
      else 
        connections_.clear();
      ul_connections.unlock();
      std::cout << "WebsocketFrame::OnClose: Connection closed successfully." << std::endl;
    } catch (std::exception& e) {
      std::cout << "WebsocketFrame::OnClose: Failed to close connection: " << e.what() << std::endl;
    }
  }
  else 
    std::cout << "WebsocketFrame::OnClose: Connection closed, but connection didn't exist!" << std::endl;
}


void WebsocketServer::on_message(server* srv, websocketpp::connection_hdl hdl, message_ptr msg) {}

void WebsocketServer::SendFieldToAllConnections(std::string payload) {
  for (const auto& it : connections_) {
    server_.send(it.second, payload, websocketpp::frame::opcode::value::text);
  }
}
