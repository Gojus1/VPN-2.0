#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <thread>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::thread;
using std::ostringstream;
using std::istringstream;
using std::stoi;
using std::thread;
using std::unique_ptr;
using std::max;

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

string resolve_host(const string& host) {
    hostent* server = gethostbyname(host.c_str());
    if (!server) {
        cerr << "[ERROR] Could not resolve " << host << "\n";
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
        if (request.find("\r\n\r\n") != string::npos) break;
    }

    if (request.empty()) {
        CLOSESOCKET(client_fd);
        return;
    }

    cerr << "[DEBUG] Received:\n" << request << "\n"; // THIS FOR TESTING

    istringstream req_stream(request);
    string first_line;
    getline(req_stream, first_line);
    if (!first_line.empty() && first_line.back() == '\r')
        first_line.pop_back();

    string host = "httpbin.org";
    int port = 80;

//For HTTPS
///////////////////////////////////////////////////////////////
if (first_line.rfind("CONNECT", 0) == 0) {
    size_t host_start = first_line.find(' ') + 1;
    size_t host_end = first_line.find(' ', host_start);
    std::string target = first_line.substr(host_start, host_end - host_start);

    size_t sep = target.find(':');
    if (sep != string::npos) {
        host = target.substr(0, sep);
        port = stoi(target.substr(sep + 1));
    } else {
        host = target;
        port = 443;
    }

    // Resolve and connect to target
    string dest_ip = resolve_host(host);
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
        CLOSESOCKET(client_fd);
        return;
    }
    const char* response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(client_fd, response, strlen(response), 0);

    fd_set fds;
    char tunnel_buf[BUFSIZE];

    while (true) {
        FD_ZERO(&fds);
        FD_SET(client_fd, &fds);
        FD_SET(forward_fd, &fds);
        int max_fd = max(client_fd, forward_fd) + 1;

        if (select(max_fd, &fds, nullptr, nullptr, nullptr) < 0)
            break;
        if (FD_ISSET(client_fd, &fds)) {
            int n = recv(client_fd, tunnel_buf, BUFSIZE, 0);
            if (n <= 0) break;
            send(forward_fd, tunnel_buf, n, 0);
        }
        if (FD_ISSET(forward_fd, &fds)) {
            int n = recv(forward_fd, tunnel_buf, BUFSIZE, 0);
            if (n <= 0) break;
            send(client_fd, tunnel_buf, n, 0);
        }
    }

    CLOSESOCKET(forward_fd);
    CLOSESOCKET(client_fd);
    return;
}
    
    //For other clients like from browser
///////////////////////////////////////////////////////////////////////////////
    if (first_line.rfind("GET", 0) == 0 || first_line.rfind("POST", 0) == 0) {
        string line;
        while (getline(req_stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (line.find("Host:") == 0 || line.find("host:") == 0) {
                host = line.substr(5);
                host.erase(0, host.find_first_not_of(" \t\r\n"));
                host.erase(host.find_last_not_of(" \t\r\n") + 1);
                break;
            }
        }

        size_t method_end = first_line.find(' ');
        string method = first_line.substr(0, method_end);

        size_t last_space = first_line.rfind(' ');
        string version = first_line.substr(last_space + 1);

        string middle = first_line.substr(method_end + 1, last_space - method_end - 1);

        string relative_path = "/";
        if (middle.rfind("http://", 0) == 0) {
            size_t slash_pos = middle.find('/', 7);
            if (slash_pos != string::npos)
                relative_path = middle.substr(slash_pos);
        } else {
            relative_path = middle;
        }
        
        first_line = method + " " + relative_path + " " + version;

        size_t first_line_end = request.find("\r\n");
        string headers = request.substr(first_line_end + 2);
        ostringstream rebuilt;
        rebuilt << first_line << "\r\n" << headers;
        request = rebuilt.str();

    } else {
        size_t sep = first_line.find(':');
        if (sep != string::npos) {
            host = first_line.substr(0, sep);
            int parsedPort = stoi(first_line.substr(sep + 1));
            if (parsedPort > 0 && parsedPort <= 65535) port = parsedPort;
        }
        request = request.substr(first_line.length() + 2);
    }
///////////////////////////////////////////////////////////////////////////////////
    cerr << "[DEBUG] Forwarding:\n" << request << "\n"; //THIS FOR TESTING

    string dest_ip = resolve_host(host);
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
        cerr << "[ERROR] Failed to create socket.\n";
        CLEANUP_NETWORK();
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "[ERROR] Failed to bind socket.\n";
        CLOSESOCKET(server_fd);
        CLEANUP_NETWORK();
        return 1;
    }

    listen(server_fd, 10);
    cout << "[VPN PROXY] Listening on port " << PORT << "...\n";

    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    socket_t client_fd;
    
    while (true) {
        client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

        if (client_fd == INVALID_SOCKET) continue;

        thread clientThread(handle_client, client_fd);
        clientThread.detach();
        
    }

    CLOSESOCKET(server_fd);
    CLEANUP_NETWORK();
    return 0;
}
