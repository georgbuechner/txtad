/**
 * @author fux
 */

#ifndef SERVER_SERVERFRAME_H_
#define SERVER_SERVERFRAME_H_

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

class ServerFrame {
  public: 

    /**
     * @brief Constructor
     * @param user_manager (pointer to user_manager)
     */
    ServerFrame(std::string path_to_cert = "", std::string path_to_key = "");

    /**
     * @brief Starts server and initializes listeners
     * @param port (port in which to start on)
     */
    int Start(int port);
  
    /**
     * @brief Gives feedback on whether server is still running
     * @return boolean (running/ not running)
     */
    bool IsRunning();

    /**
     * @brief Makes server stop running.
     */
    void Stop();

    /**
     * @brief Destructor, which stops server.
     */
    ~ServerFrame();

  private: 
    
    //*** Membervariables *** // 
#ifdef _DEVELOPMENT_
    httplib::SSLServer server_;
#else
    httplib::Server server_;  //Server
#endif
};

#endif

