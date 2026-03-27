#include <iostream>
#include <string>
#include <thread>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSESOCKET closesocket
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
typedef int socket_t;
#define CLOSESOCKET close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#define PORT 5000
#define BUFFER_SIZE 8192

void forward_data(socket_t a, socket_t b)
{
    char buffer[BUFFER_SIZE];
    int len;

    while ((len = recv(a, buffer, BUFFER_SIZE, 0)) > 0)
    {
        send(b, buffer, len, 0);
    }

    CLOSESOCKET(a);
    CLOSESOCKET(b);
}

void handle_client(socket_t client)
{
    char buffer[BUFFER_SIZE];
    int len = recv(client, buffer, BUFFER_SIZE - 1, 0);

    if (len <= 0)
    {
        CLOSESOCKET(client);
        return;
    }

    buffer[len] = '\0';
    std::string request(buffer);

// HTTPS

    if (request.rfind("CONNECT", 0) == 0)
    {
        size_t host_start = 8;
        size_t host_end = request.find(' ', host_start);

        std::string host_port = request.substr(host_start, host_end - host_start);

        size_t colon = host_port.find(':');
        std::string host = host_port.substr(0, colon);
        int port = std::stoi(host_port.substr(colon + 1));

        hostent* server = gethostbyname(host.c_str());
        if (!server)
        {
            CLOSESOCKET(client);
            return;
        }

        socket_t remote = socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr = *(in_addr*)server->h_addr;

        if (connect(remote, (sockaddr*)&addr, sizeof(addr)) < 0)
        {
            CLOSESOCKET(client);
            return;
        }

        std::string ok = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(client, ok.c_str(), ok.size(), 0);

        std::thread t1(forward_data, client, remote);
        std::thread t2(forward_data, remote, client);

        t1.detach();
        t2.detach();

        return;
    }

// HTTP
    size_t host_pos = request.find("http://");
    if (host_pos == std::string::npos)
    {
        CLOSESOCKET(client);
        return;
    }

    host_pos += 7;
    size_t path_pos = request.find('/', host_pos);

    std::string host = request.substr(host_pos, path_pos - host_pos);
    std::string path = request.substr(path_pos);

    size_t colon = host.find(':');
    int port = 80;

    if (colon != std::string::npos)
    {
        port = std::stoi(host.substr(colon + 1));
        host = host.substr(0, colon);
    }

    hostent* server = gethostbyname(host.c_str());
    if (!server)
    {
        CLOSESOCKET(client);
        return;
    }

    socket_t remote = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *(in_addr*)server->h_addr;

    if (connect(remote, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        CLOSESOCKET(client);
        return;
    }

    std::string new_request = request;
    size_t first_space = new_request.find(' ');
    size_t second_space = new_request.find(' ', first_space + 1);

    new_request.replace(first_space + 1,
                        second_space - first_space - 1,
                        path);

    send(remote, new_request.c_str(), new_request.size(), 0);

    std::thread t(forward_data, remote, client);
    t.detach();
}

int main()
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    socket_t server = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    listen(server, 20);

    std::cout << "HTTP/HTTPS Proxy running on port " << PORT << std::endl;

    while (true)
    {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        socket_t client = accept(server, (sockaddr*)&client_addr, &len);

        if (client != INVALID_SOCKET)
        {
            std::thread(handle_client, client).detach();
        }
    }

    CLOSESOCKET(server);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}