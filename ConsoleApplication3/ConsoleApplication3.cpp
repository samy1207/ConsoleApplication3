#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <stdexcept>

#pragma comment(lib, "ws2_32.lib")

class Comms {
private:
    WSADATA wsaData;
    SOCKET clientSocket;
    SOCKET serverSocket;
    sockaddr_in serverAddr;
    bool isClient;

public:
    Comms(bool isClient) : isClient(isClient), clientSocket(INVALID_SOCKET), serverSocket(INVALID_SOCKET), serverAddr{}, wsaData{} {}

    ~Comms() {
        closeConnection();
    }

    void initialize() {
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("Failed to initialize Winsock.");
        }
    }

    void createSocket() {
        if (isClient) {
            clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (clientSocket == INVALID_SOCKET) {
                throw std::runtime_error("Failed to create client socket.");
            }
        }
        else {
            serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (serverSocket == INVALID_SOCKET) {
                throw std::runtime_error("Failed to create server socket.");
            }
        }
    }

    bool connectToServer(const std::string& ipAddress, int port) {
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr);

        if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            return false;
        }

        return true;
    }

    void bindAndListen(int port) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(serverSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to bind to port.");
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to listen on socket.");
        }
    }

    SOCKET acceptClient() {
        SOCKET client = accept(serverSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            throw std::runtime_error("Failed to accept client connection.");
        }
        return client;
    }

    void sendMessage(const std::string& message) {
        if (send(clientSocket, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send message.");
        }
    }

    std::string receiveMessage() {
        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived == SOCKET_ERROR) {
            throw std::runtime_error("Failed to receive message.");
        }
        buffer[bytesReceived] = '\0';
        return std::string(buffer);
    }

    void closeConnection() {
        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();
    }

    bool isClientMode() const {
        return isClient;
    }
};

class Client {
private:
    Comms comms;

public:
    Client() : comms(true) {}

    void run() {
        try {
            comms.initialize();
            comms.createSocket();
            std::string ipAddress;
            int port;
            std::cout << "Enter server IP address: ";
            std::cin >> ipAddress;
            std::cout << "Enter server port number: ";
            std::cin >> port;
            if (comms.connectToServer(ipAddress, port)) {
                std::cout << "Connected to server." << std::endl;
                std::cout << "Client mode" << std::endl;

                std::string message;
                while (true) {
                    std::cout << "Enter message (type QUIT to exit): ";
                    std::getline(std::cin, message);
                    if (message == "QUIT")
                        break;
                    comms.sendMessage(message);
                    std::string receivedMessage = comms.receiveMessage();
                    std::cout << "Server: " << receivedMessage << std::endl;
                }

                comms.closeConnection();
            }
            else {
                std::cerr << "Failed to connect to server." << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Client error: " << e.what() << std::endl;
            std::cerr << "Failed to connect to server." << std::endl;
        }
    }
};

class Server {
private:
    Comms comms;

public:
    Server() : comms(false) {}

    void run() {
        try {
            comms.initialize();
            comms.createSocket();
            int port;
            std::cout << "Enter server port number: ";
            std::cin >> port;
            comms.bindAndListen(port);
            std::cout << "Server is listening for connections..." << std::endl;
            std::cout << "Server mode" << std::endl;

            SOCKET clientSocket = comms.acceptClient();
            if (clientSocket != INVALID_SOCKET) {
                std::cout << "Client connected." << std::endl;

                std::string message;
                while (true) {
                    message = comms.receiveMessage();
                    std::cout << "Client: " << message << std::endl;
                    std::cout << "Enter message (type QUIT to exit): ";
                    std::getline(std::cin, message);
                    if (message == "QUIT")
                        break;
                    comms.sendMessage(message);
                }

                comms.closeConnection();
            }
            else {
                std::cerr << "Failed to accept client connection." << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Server error: " << e.what() << std::endl;
            std::cerr << "Failed to listen for connections." << std::endl;
        }
    }
};

int main() {
    std::cout << "Choose mode: " << std::endl;
    std::cout << "1. Client" << std::endl;
    std::cout << "2. Server" << std::endl;
    int choice;
    std::cin >> choice;

    if (choice == 1) {
        Client client;
        client.run();
    }
    else if (choice == 2) {
        Server server;
        server.run();
    }
    else {
        std::cerr << "Invalid choice. Exiting..." << std::endl;
    }

    std::cout << "Press any key to exit..." << std::endl;
    std::cin.get();
    return 0;
}
