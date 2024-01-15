#include "server.h"

TemperatureServer::TemperatureServer(DatabaseManager* dbManager) {
    this->dbManager = dbManager;
}

TemperatureServer::~TemperatureServer() {
    JoinThread();
}

SOCKET TemperatureServer::Listen(short int port) {
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        std::cout << "Failed to open socket: " << ErrorCode() << std::endl;
        return INVALID_SOCKET;
    }

    sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port);

    if (bind(m_socket, (struct sockaddr*)&local_addr, sizeof(local_addr)) == SOCKET_ERROR) {
        std::cout << "Failed to bind: " << ErrorCode() << std::endl;
        CloseMySocket();
        return INVALID_SOCKET;
    }

    if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Failed to start listen: " << ErrorCode() << std::endl;
        CloseMySocket();
        return INVALID_SOCKET;
    }

    return m_socket;
}

void TemperatureServer::HandleClientRequest(SOCKET clientSocket) {
    std::ostringstream jsonResponse;
    std::string request = std::string(inputBuf);

    if (request.find("/current_temperature") != std::string::npos) {
        int currentTemperature = dbManager->getCurrentTemperature();
        std::cout << "dbManager->getCurrentTemperature() returns: " << currentTemperature << std::endl;
        jsonResponse << "{\"currentTemperature\":" << currentTemperature << "}";
    } else if (request.find("GET /statistics?days=") != std::string::npos) {
        size_t start = request.find('=') + 1;
        size_t end = request.find(' ', start);
        if (start != std::string::npos && end != std::string::npos) {
            std::string period = request.substr(start, end - start);
            int days = atoi(period.c_str());
            auto temperatureData = dbManager->getTemperatureDataFromLastDays(days);
            if (!temperatureData.second.empty()) {
                jsonResponse << "{\"averageTemperature\":" << temperatureData.first << ",\"temperatureData\":[";
                for (size_t i = 0; i < temperatureData.second.size(); ++i) {
                    jsonResponse << "{\"day number\":" << static_cast<int>(temperatureData.second[i]) << "}";
                    if (i < temperatureData.second.size() - 1) {
                        jsonResponse << ",";
                    }
                }
                jsonResponse << "]}";
            }
        }
    } else {
        jsonResponse << "{\"error\":\"Invalid Request\"}";
    }
    SendHttpResponse(jsonResponse.str(), clientSocket);
}

void TemperatureServer::AcceptClient() {
    SOCKET clientSocket = accept(m_socket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Error accepting client: " << ErrorCode() << std::endl;
        CloseSocket(clientSocket);
        return;
    }
    struct pollfd polstr;
    memset(&polstr, 0, sizeof(polstr));
    polstr.fd = clientSocket;
    polstr.events |= POLLIN;

#ifdef WIN32
        int ret = WSAPoll(&polstr, 1, READ_WAIT_MS);
#else
        int ret = poll(&polstr, 1, READ_WAIT_MS);
#endif

    if (ret <= 0) {
        CloseSocket(clientSocket);
        return;
    }
    int result = recv(clientSocket, inputBuf, sizeof(inputBuf), 0);
    if (result == SOCKET_ERROR) {
        std::cout << "Error on client receive: " << result << std::endl;
        CloseSocket(clientSocket);
        return;
    } else if (result == 0) {
        std::cout << "Client closed connection before getting any data!" << std::endl;
        CloseSocket(clientSocket);
        return;
    }

    HandleClientRequest(clientSocket);
    CloseSocket(clientSocket);
    std::cout << "Answered to client!" << std::endl;
}

void TemperatureServer::RunServer(SOCKET serverSocket) {
    while (true) {
        if (serverSocket != INVALID_SOCKET) {
            AcceptClient();
        }
    }
}

void TemperatureServer::StartThread(SOCKET serverSocket) {
    serverThread = std::thread([this, serverSocket]() {
        RunServer(serverSocket);
    });
}

void TemperatureServer::JoinThread() {
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

void TemperatureServer::SendHttpResponse(const std::string& response, SOCKET clientSocket) {
    std::stringstream responseStream;
    responseStream << "HTTP/1.0 200 OK\r\n"
                   << "Version: HTTP/1.1\r\n"
                   << "Content-Type: application/json; charset=utf-8\r\n"  // Изменено на application/json
                   << "Content-Length: " << response.length()
                   << "\r\n\r\n"
                   << response;
    
    int result = send(clientSocket, responseStream.str().c_str(), (int)responseStream.str().length(), 0);
    if (result == SOCKET_ERROR) {
        std::cout << "Failed to send response to client: " << ErrorCode() << std::endl;
    }
}

SocketWorker::SocketWorker() : m_socket(INVALID_SOCKET) {}

SocketWorker::~SocketWorker() {
    CloseMySocket();
}

int SocketWorker::ErrorCode() {
#if defined (WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
}

void SocketWorker::CloseMySocket() {
    CloseSocket(m_socket);
    m_socket = INVALID_SOCKET;
}

void SocketWorker::CloseSocket(SOCKET sock) {
    if (sock == INVALID_SOCKET)
        return;
#if defined (WIN32)
        shutdown(sock, SD_SEND);
        closesocket(sock);
#else
        shutdown(sock, SHUT_WR);
        close(sock);
#endif
}
