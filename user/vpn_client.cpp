#include "vpn_client.h"
#include <iostream>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSESOCKET closesocket
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define CLOSESOCKET ::close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

Client::Client(const Flags& config) {

#ifdef _WIN32
    WSAData wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "Winsock init failed\n";
        return;
    }
#endif

    std::string ip =
        std::to_string(config.ipv4[0]) + "." +
        std::to_string(config.ipv4[1]) + "." +
        std::to_string(config.ipv4[2]) + "." +
        std::to_string(config.ipv4[3]);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(config.port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        return;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return;
    }

    std::cout << "Connecting to VPN server at " << ip << "...\n";

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        perror("connect");
        CLOSESOCKET(sock);
        return;
    }

    std::cout << "Connected to VPN server\n";

    std::string target = "httpbin.org:80\r\n";
    send(sock, target.c_str(), target.size(), 0);

    std::string request =
        "GET /ip HTTP/1.1\r\n"
        "Host: httpbin.org\r\n"
        "Connection: close\r\n\r\n";

    send(sock, request.c_str(), request.size(), 0);

    char buffer[4096];
    int bytesReceived;

    std::cout << "\nResponse:\n\n";

    while ((bytesReceived = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << buffer;
    }

    CLOSESOCKET(sock);

#ifdef _WIN32
    WSACleanup();
#endif
}

void Client::establishConnection() {}
void Client::close() {}