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
    int len = recv(client_fd, buffer, BUFSIZE - 1, 0);
    if (len <= 0) return;
    buffer[len] = '\0';

    std::istringstream iss(buffer);
    std::string first_line;
    std::getline(iss, first_line);

    std::string host = "httpbin.org";
    int port = 80;

    // To check clients request
    if (first_line.rfind("GET", 0) == 0 || first_line.rfind("POST", 0) == 0) {
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("Host:") == 0) {
                host = line.substr(5);
                while (!host.empty() && (host.front() == ' ' || host.front() == '\t'))
                    host.erase(host.begin());
                if (!host.empty() && host.back() == '\r')
                    host.pop_back();
                break;
            }
        }
    } else {
        size_t sep = first_line.find(':');
        if (sep != std::string::npos) {
            host = first_line.substr(0, sep);
            int parsedPort = std::stoi(first_line.substr(sep + 1));
            if (parsedPort > 0 && parsedPort <= 65535)
                port = parsedPort;
        }
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

    send(forward_fd, buffer, len, 0);

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

