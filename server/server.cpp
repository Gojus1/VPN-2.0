#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>

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
#define CERT_FILE "../cert.pem"
#define KEY_FILE "../key.pem"

void forward_data(socket_t a, socket_t b, SSL* ssl_a = nullptr, SSL* ssl_b = nullptr)
{
    char buffer[BUFFER_SIZE];
    int len;

    while (true)
    {
        if (ssl_a)
            len = SSL_read(ssl_a, buffer, BUFFER_SIZE);
        else
            len = recv(a, buffer, BUFFER_SIZE, 0);

        if (len <= 0) break;

        if (ssl_b)
            SSL_write(ssl_b, buffer, len);
        else
            send(b, buffer, len, 0);
    }

    if (!ssl_a) CLOSESOCKET(a);
    if (!ssl_b) CLOSESOCKET(b);
}

// Handle a single client connection (SSL-wrapped)
void handle_client(SSL* ssl_client, socket_t raw_client)
{
    char buffer[BUFFER_SIZE];
    int len = SSL_read(ssl_client, buffer, BUFFER_SIZE - 1);

    if (len <= 0)
    {
        SSL_shutdown(ssl_client);
        SSL_free(ssl_client);
        CLOSESOCKET(raw_client);
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
            SSL_shutdown(ssl_client);
            SSL_free(ssl_client);
            CLOSESOCKET(raw_client);
            return;
        }

        socket_t remote = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr = *(in_addr*)server->h_addr;

        if (connect(remote, (sockaddr*)&addr, sizeof(addr)) < 0)
        {
            SSL_shutdown(ssl_client);
            SSL_free(ssl_client);
            CLOSESOCKET(raw_client);
            return;
        }

        std::string ok = "HTTP/1.1 200 Connection Established\r\n\r\n";
        SSL_write(ssl_client, ok.c_str(), ok.size());

        std::thread t1(forward_data, raw_client, remote, ssl_client, nullptr);
        std::thread t2(forward_data, remote, raw_client, nullptr, ssl_client);

        t1.detach();
        t2.detach();
        return;
    }

    // HTTP
    size_t host_pos = request.find("http://");
    if (host_pos == std::string::npos)
    {
        SSL_shutdown(ssl_client);
        SSL_free(ssl_client);
        CLOSESOCKET(raw_client);
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
        SSL_shutdown(ssl_client);
        SSL_free(ssl_client);
        CLOSESOCKET(raw_client);
        return;
    }

    socket_t remote = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *(in_addr*)server->h_addr;

    if (connect(remote, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        SSL_shutdown(ssl_client);
        SSL_free(ssl_client);
        CLOSESOCKET(raw_client);
        return;
    }

    std::string new_request = request;
    size_t first_space = new_request.find(' ');
    size_t second_space = new_request.find(' ', first_space + 1);
    new_request.replace(first_space + 1, second_space - first_space - 1, path);

    send(remote, new_request.c_str(), new_request.size(), 0);

    std::thread t(forward_data, remote, raw_client, nullptr, ssl_client);
    t.detach();
}

int main()
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    // OpenSSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        std::cerr << "Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return 1;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return 1;
    }

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
    std::cout << "TLS HTTP/HTTPS Proxy running on port " << PORT << std::endl;

    while (true)
    {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        socket_t client = accept(server, (sockaddr*)&client_addr, &len);

        if (client == INVALID_SOCKET) continue;

        SSL* ssl_client = SSL_new(ctx);
        SSL_set_fd(ssl_client, client);

        if (SSL_accept(ssl_client) <= 0)
        {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl_client);
            CLOSESOCKET(client);
            continue;
        }

        std::thread(handle_client, ssl_client, client).detach();
    }

    CLOSESOCKET(server);
    SSL_CTX_free(ctx);

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}