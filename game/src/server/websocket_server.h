/**
 * @author: fux
 */

#ifndef SERVER_WEBSOCKET_SERVER_H_
#define SERVER_WEBSOCKET_SERVER_H_

#include <iostream>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

class WebsocketServer {
  public:
     ///< Connection_id-typedef.
    typedef decltype(websocketpp::lib::weak_ptr<void>().lock().get()) connection_id;

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

    /**
     * Initializes and starts main loop. (THREAD)
     * @param[in] port 
     */
    void Start(int port);

    /**
     * Send json to als connections
     */
    void SendFieldToAllConnections(std::string payload);

    /**
     * Closes games every 5 seconds if finished. (THREAD)
     * (if standalone_, closes thread)
     * Updates lobby if a game is closed.
     */
    void CloseGames();

  private:

    // typedefs:
    typedef websocketpp::server<websocketpp::config::asio> server;
    typedef server::message_ptr message_ptr;
    typedef std::pair<int, std::string> error_obj;
    
    // members:

    server server_;  ///< server object.
    mutable std::shared_mutex mutex_connections_;  ///< Mutex for connections_.
    std::map<connection_id, websocketpp::connection_hdl> connections_;  ///< Dictionary with all connections.

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
    void on_message(server* srv, websocketpp::connection_hdl hdl, message_ptr msg);
};

#endif 
