#pragma once
#include "util/flags.h"

class Client {
public:
    Client(const Flags& flags);
    void establishConnection();
    void close();
};