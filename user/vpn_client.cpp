#include "vpn_client.h"

#pragma comment(lib, "ws2_32.lib")

// class for client logic
Client::Client(const Flags& config) {
    WSAData wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "Failed to initialize Winsock.\n";
    }

    std::string ip = std::to_string(config.ipv4[0]) + "." +
                     std::to_string(config.ipv4[1]) + "." +
                     std::to_string(config.ipv4[2]) + "." +
                     std::to_string(config.ipv4[3]);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(config.port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket.\n";
        WSACleanup();
        return;
    }

    std::cout << "Connecting to VPN server at " << ip << "...\n";
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection to proxy failed.\n";
        closesocket(sock);
        WSACleanup();
        return;
    }
    std::cout << "Connected\n";

    std::string target = "httpbin.org:80\n"; // destination that server will resolve
    send(sock, target.c_str(), target.size(), 0);
    std::cout << "Client's request: " << target;

    std::string request =
        "GET /ip HTTP/1.1\r\n"
        "Host: httpbin.org\r\n"
        "Connection: close\r\n\r\n";

    send(sock, request.c_str(), request.size(), 0);

    char buffer[4096];
    int bytesReceived;

    std::cout << "\nResponse: \n\n";
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << buffer;
    }

    closesocket(sock);
    WSACleanup();
}

void Client::establishConnection() {}
void Client::close() {}
