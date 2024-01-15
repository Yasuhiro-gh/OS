#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include "db_manager.h"

#if defined (WIN32)
#   include <winsock2.h>    /* socket */
#   include <ws2tcpip.h>    /* ipv6 */
#else
#   include <sys/socket.h>  /* socket */
#   include <netinet/in.h>  /* socket */
#   include <arpa/inet.h>   /* socket */
#   include <unistd.h>      
#   define SOCKET int
#   define INVALID_SOCKET -1
#   define SOCKET_ERROR -1
#endif

#define READ_WAIT_MS 50

class SocketWorker {
public:
    SocketWorker();
    ~SocketWorker();

    static int ErrorCode();

protected:
    void CloseMySocket();
    void CloseSocket(SOCKET sock);

    SOCKET m_socket;
};

class TemperatureServer : public SocketWorker {
public:
    TemperatureServer(DatabaseManager* dbManager);
    ~TemperatureServer();

    SOCKET Listen(short int port);
    void HandleClientRequest(SOCKET clientSocket);
    void AcceptClient();
    void RunServer(SOCKET serverSocket);
    void StartThread(SOCKET serverSocket);
    void JoinThread();

private:
    void SendHttpResponse(const std::string& response, SOCKET clientSocket);

    char inputBuf[1024];
    std::thread serverThread;
    DatabaseManager* dbManager;
};
