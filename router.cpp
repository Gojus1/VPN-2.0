#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

#define PORT 5000
#define BUFSIZE 8192

void pipe_data(int from, int to) {
    char buffer[BUFSIZE];
    int len;
    while ((len = read(from, buffer, BUFSIZE)) > 0) {
        send(to, buffer, len, 0);
    }
    shutdown(to, SHUT_WR);
}

void handle_client(int client_fd) {
    char line[BUFSIZE];
    int idx = 0;
    char ch;
    
    // Read the host:port line
    while (read(client_fd, &ch, 1) == 1 && ch != '\n' && idx < BUFSIZE - 1) {
        line[idx++] = ch;
    }
    line[idx] = '\0';

    std::string dest(line);
    size_t colon = dest.find(':');
    if (colon == std::string::npos) {
        std::cerr << "[ERROR] Invalid destination format\n";
        close(client_fd);
        return;
    }

    std::string hostname = dest.substr(0, colon);
    int port = std::stoi(dest.substr(colon + 1));

    // Resolve host
    hostent* server = gethostbyname(hostname.c_str());
    if (!server) {
        std::cerr << "[ERROR] Could not resolve host\n";
        close(client_fd);
        return;
    }

    sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    memcpy(&dest_addr.sin_addr, server->h_addr, server->h_length);

    int forward_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(forward_fd, (sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
        std::cerr << "[ERROR] Could not connect to destination\n";
        close(client_fd);
        return;
    }

    // Start piping data
    std::thread t1(pipe_data, client_fd, forward_fd);
    std::thread t2(pipe_data, forward_fd, client_fd);
    t1.join();
    t2.join();

    close(forward_fd);
    close(client_fd);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    std::cout << "[VPN PROXY] Listening on port " << PORT << "\n";

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd >= 0) {
            std::thread(handle_client, client_fd).detach();
        }
    }

    close(server_fd);
    return 0;
}
