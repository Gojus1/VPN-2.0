#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "IP_PASTE"
#define SERVER_PORT 5000
#define DEST_HOST "httpbin.org"
#define DEST_PORT 80
#define BUFFER_SIZE 4096

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "Failed to initialize Winsock.\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    std::cout << "Connecting to VPN server at " << SERVER_IP << "...\n";
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection to proxy failed.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    std::cout << "Connected\n";

    std::string target = std::string(DEST_HOST) + ":" + std::to_string(DEST_PORT) + "\n";
    send(sock, target.c_str(), target.size(), 0);
    std::cout << "Clients request: " << target;

    std::string request =
        "GET /ip HTTP/1.1\r\n"
        "Host: " + std::string(DEST_HOST) + "\r\n"
        "Connection: close\r\n\r\n";

    send(sock, request.c_str(), request.size(), 0);

    char buffer[BUFFER_SIZE];
    int bytesReceived;

    std::cout << "\nResponse: \n\n";
    while ((bytesReceived = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << buffer;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
