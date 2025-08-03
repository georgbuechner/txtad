/**
 * @author: fux
 */

#ifndef SERVER_WEBSOCKET_SERVER_H_
#define SERVER_WEBSOCKET_SERVER_H_

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

class WebsocketServer {
  public:
    using EventHandlerFn = std::function<void(std::string, std::string, std::string)>;
     ///< Connection_id-typedef.
    typedef decltype(websocketpp::lib::weak_ptr<void>().lock().get()) t_connection_id;

    // public methods:
   
    /**
     * Constructs websocket-server.
     * @param[in] standalone
     * @param[in] base_path 
     */
    WebsocketServer();

    /**
     * Destructor stoping websocket server
     */
    ~WebsocketServer();

    // setter 
    static void set_handle_event(EventHandlerFn fn);

    /**
     * Initializes and starts main loop. (THREAD)
     * @param[in] port 
     */
    void Start(int port);

    /**
     * Send payload to given user
     * @param[in] user_id 
     * @param[in] payload
     */
    void SendMessage(const std::string& user_id, const std::string& payload);

  private:

    // typedefs:
    typedef websocketpp::server<websocketpp::config::asio> t_server;
    typedef t_server::message_ptr t_message_ptr;
    
    // members:

    t_server _server;  ///< server object.
    mutable std::shared_mutex _mutex;  ///< Mutex for connections_.
    std::map<std::string, websocketpp::connection_hdl> _connections;  ///< Dictionary with all connections.
    std::map<std::string, std::string> _user_game_mapping;   ///< maps user-id to game (for `OnClose` access)

    static EventHandlerFn _handle_event;

    // methods:
    
    /**
     * Opens a new connection and adds entry in dictionary of connections.
     * Until the connection is futher initialized it will be set to a
     * Connection. Later this is deleted and replaced with ControllerConnection,
     * UserOverviewConnection or UserControllerConnection.
     * @param hdl[in] websocket-connection.
     */
    void OnOpen(websocketpp::connection_hdl hdl);

    /**
     * Closes an active connection and removes connection from connections.
     * @param[in] hdl websocket-connection.
     */
    void OnClose(websocketpp::connection_hdl hdl);

    /**
     * Handles incomming requests/ commands.
     * @param[in] srv pointer to server.
     * @param[in] hdl incomming connection.
     * @param[in] msg message.
     */
    void OnMessage(t_server* srv, websocketpp::connection_hdl hdl, t_message_ptr msg);

    static const std::string ConnectionIDToString(t_connection_id connection_id);
};

#endif 
