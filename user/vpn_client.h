#pragma once
#include "../util/flags.h"
#include <winsock2.h>
#include <ws2tcpip.h>


class Client {
public:
    Client(const Flags& flags);
    void establishConnection();
    void close();
};