#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>

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
void handle_client(int client_fd) {
    char buffer[BUFSIZE];
    std::string line;
    int len = recv(client_fd, buffer, BUFSIZE - 1, 0);
    if (len <= 0) return;
    buffer[len] = '\0';

    std::istringstream iss(buffer);
    std::getline(iss, line);

    std::string host;
    int port = 80;

    size_t sep = line.find(':');
    if (sep != std::string::npos) {
        host = line.substr(0, sep);
        std::string portPart = line.substr(sep + 1);
        int parsedPort = 0;
        for (char c : portPart) {
            if (c >= '0' && c <= '9') {
                parsedPort = parsedPort * 10 + (c - '0');
            } else {
                break;
            }
        }
        if (parsedPort > 0 && parsedPort <= 65535) {
            port = parsedPort;
        } else {
            std::cerr << "[ERROR] Invalid port format. Using default 80\n";
        }
    } else {
        std::cerr << "[ERROR] Invalid format. Using default values\n";
        host = "httpbin.org";
    }


    std::string dest_ip = resolve_host(host);
    if (dest_ip.empty()) {
        close(client_fd);
        return;
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    inet_pton(AF_INET, dest_ip.c_str(), &dest.sin_addr);

    int forward_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(forward_fd, (sockaddr*)&dest, sizeof(dest)) < 0) {
        std::cerr << "[ERROR] Could not connect to " << dest_ip << "\n";
        close(client_fd);
        return;
    }

    std::string full_request;

    std::string leftover;
    std::getline(iss, line);
    while (std::getline(iss, line)) {
        leftover += line + "\n";
    }
    full_request += leftover;

    while (full_request.find("\r\n\r\n") == std::string::npos) {
        len = recv(client_fd, buffer, BUFSIZE - 1, 0);
        if (len <= 0) {
            std::cerr << "[ERROR] Client closed before sending request.\n";
            close(client_fd);
            return;
        }
        buffer[len] = '\0';
        full_request += buffer;
    }

    send(forward_fd, full_request.c_str(), full_request.size(), 0);

    while ((len = recv(forward_fd, buffer, BUFSIZE, 0)) > 0) {
        send(client_fd, buffer, len, 0);
    }

    close(forward_fd);
    close(client_fd);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[ERROR] Failed to create socket.\n";
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[ERROR] Failed to bind socket.\n";
        return 1;
    }

    listen(server_fd, 10);
    std::cout << "[VPN PROXY] Listening on port " << PORT << "...\n";

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd >= 0) {
            if (fork() == 0) {
                close(server_fd);
                handle_client(client_fd);
                exit(0);
            } else {
                close(client_fd);
            }
        }
    }

    close(server_fd);
    return 0;
}

