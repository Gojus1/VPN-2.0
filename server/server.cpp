#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <thread>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    #define CLOSESOCKET closesocket
    #define INIT_NETWORK() \
        WSADATA wsa; \
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { \
            std::cerr << "[ERROR] WSAStartup failed.\n"; \
            return 1; \
        }
    #define CLEANUP_NETWORK() WSACleanup()
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define CLOSESOCKET close
    #define INIT_NETWORK()
    #define CLEANUP_NETWORK()
#endif

#define PORT 5000
#define BUFSIZE 8192

std::string resolve_host(const std::string& host) {
    hostent* server = gethostbyname(host.c_str());
    if (!server) {
        std::cerr << "[ERROR] Could not resolve " << host << "\n";
        return "";
    }
    return inet_ntoa(*(struct in_addr*)server->h_addr);
}

void handle_client(socket_t client_fd) {
    char buffer[BUFSIZE];
    std::string request;

    int len;
    while ((len = recv(client_fd, buffer, BUFSIZE - 1, 0)) > 0) {
        buffer[len] = '\0';
        request += buffer;
        if (request.find("\r\n\r\n") != std::string::npos) break;
    }

    if (request.empty()) {
        CLOSESOCKET(client_fd);
        return;
    }

       std::cerr << "[DEBUG] Received:\n" << request << "\n"; //FOR TESTING

    std::istringstream req_stream(request);
    std::string first_line;
    std::getline(req_stream, first_line);

    std::string host = "httpbin.org";
    int port = 80;

    //Need this for other clients
//////////////////////////////////////////////////////////////////////////////////
    if (first_line.rfind("GET", 0) == 0 || first_line.rfind("POST", 0) == 0) {
        std::string line;
        while (std::getline(req_stream, line)) {
            if (line.find("Host:") == 0) {
                host = line.substr(5);
                while (!host.empty() && (host.front() == ' ' || host.front() == '\t'))
                    host.erase(host.begin());
                if (!host.empty() && host.back() == '\r')
                    host.pop_back();
                break;
            }
        }

        size_t http_pos = first_line.find("http://");
        if (http_pos != std::string::npos) {
            size_t path_pos = first_line.find('/', http_pos + 7);
            if (path_pos != std::string::npos) {
                std::string method = first_line.substr(0, first_line.find(' '));
                std::string relative_path = first_line.substr(path_pos);
                std::string version = first_line.substr(first_line.rfind(' '));
                first_line = method + " " + relative_path + version;
            }
        }

        size_t first_line_end = request.find("\r\n");
        std::string headers = request.substr(first_line_end + 2);
        std::ostringstream rebuilt;
        rebuilt << first_line << "\r\n" << headers;
        request = rebuilt.str();
    } else {
        size_t sep = first_line.find(':');
        if (sep != std::string::npos) {
            host = first_line.substr(0, sep);
            int parsedPort = std::stoi(first_line.substr(sep + 1));
            if (parsedPort > 0 && parsedPort <= 65535) port = parsedPort;
        }
        request = request.substr(first_line.length() + 1);
    }

        std::cerr << "[DEBUG] Forwarding:\n" << request << "\n"; // FOR TESTING
//////////////////////////////////////////////////////////////////////////////////
    std::string dest_ip = resolve_host(host);
    if (dest_ip.empty()) {
        CLOSESOCKET(client_fd);
        return;
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    inet_pton(AF_INET, dest_ip.c_str(), &dest.sin_addr);

    socket_t forward_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(forward_fd, (sockaddr*)&dest, sizeof(dest)) < 0) {
       // std::cerr << "[ERROR] Could not connect to " << dest_ip << "\n";
        CLOSESOCKET(client_fd);
        return;
    }

    send(forward_fd, request.c_str(), request.size(), 0);

    while ((len = recv(forward_fd, buffer, BUFSIZE, 0)) > 0) {
        send(client_fd, buffer, len, 0);
    }

    CLOSESOCKET(forward_fd);
    CLOSESOCKET(client_fd);
}



int main() {
    INIT_NETWORK();

    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "[ERROR] Failed to create socket.\n";
        CLEANUP_NETWORK();
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[ERROR] Failed to bind socket.\n";
        CLOSESOCKET(server_fd);
        CLEANUP_NETWORK();
        return 1;
    }

    listen(server_fd, 10);
    std::cout << "[VPN PROXY] Listening on port " << PORT << "...\n";

    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    socket_t client_fd;
    
    while (true) {
        client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

        if (client_fd == INVALID_SOCKET) continue;

        std::thread clientThread(handle_client, client_fd);
        clientThread.detach();
        
    }

    CLOSESOCKET(server_fd);
    CLEANUP_NETWORK();
    return 0;
}
