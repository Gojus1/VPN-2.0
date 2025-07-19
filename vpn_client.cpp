#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "143.110.228.242"  // <-- your proxy server IP
#define PROXY_PORT 5000
#define DEST_HOST "httpbin.org"
#define DEST_PORT 80
#define BUFSIZE 4096

DWORD WINAPI tunnel_read(LPVOID param) {
    SOCKET from = *(SOCKET*)param;
    char buffer[BUFSIZE];
    int len;
    while ((len = recv(from, buffer, BUFSIZE - 1, 0)) > 0) {
        buffer[len] = '\0';
        std::cout << buffer;
    }
    return 0;
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in proxy{};
    proxy.sin_family = AF_INET;
    proxy.sin_port = htons(PROXY_PORT);
    proxy.sin_addr.s_addr = inet_addr(SERVER_IP);

    std::cout << "Connecting to VPN proxy at " << SERVER_IP << "...\n";
    if (connect(sock, (sockaddr*)&proxy, sizeof(proxy)) < 0) {
        std::cerr << "Connection failed.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Send destination info (e.g., "httpbin.org:80\n")
    std::string connect_info = std::string(DEST_HOST) + ":" + std::to_string(DEST_PORT) + "\n";
    send(sock, connect_info.c_str(), connect_info.length(), 0);

    // Wait a moment to let the proxy connect
    Sleep(100);

    // Hardcoded HTTP request
    const char* http_request =
        "GET /ip HTTP/1.1\r\n"
        "Host: httpbin.org\r\n"
        "Connection: close\r\n"
        "\r\n";

    send(sock, http_request, strlen(http_request), 0);

    // Start reading the response
    HANDLE hReadThread = CreateThread(nullptr, 0, tunnel_read, &sock, 0, nullptr);
    WaitForSingleObject(hReadThread, INFINITE);
    CloseHandle(hReadThread);

    closesocket(sock);
    WSACleanup();
    return 0;
}
