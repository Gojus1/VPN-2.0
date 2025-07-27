#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

typedef struct Flags { //stores credentials and ipv4:port and flags such as multi vpn chaining or something
	int port;
	unsigned ipv4[4];
	std::string pass;
	bool multi; // this is for when i decide to add fragmenting the conn

	void parseIPv4(const std::string& ip) {
	    int part = 0;
	    std::string num;
	    for (char c : ip) {
	        if (c == '.') {
	            this->ipv4[part++] = std::stoi(num);
	            num.clear();
	        } else {
	            num += c;
	        }
	    }
	    if (!num.empty() && part < 4) {
	        this->ipv4[part] = std::stoi(num);
	    }
	}

	Flags() {	// basically default construtor that is empty
		port = 5000;
		ipv4[4] = {};
		pass = "";
	}

	Flags(const std::string ipv4, const std::string pass) {
		port = 5000;
		parseIPv4(ipv4);
		this->pass = pass;
	}

	Flags(const std::string ipv4, int port, const std::string pass) {
		this->port = port;
		parseIPv4(ipv4);
		this->pass = pass;
	}

	void setMulti(bool multi) {
		this->multi = multi;
	}
};