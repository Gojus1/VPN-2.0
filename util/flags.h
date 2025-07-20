#include <iostream>


struct Flags { //stores credentials and ipv4:port and flags such as multi vpn chaining or something
	int port;
	unsigned ipv4[4];
	std::string pass;
	bool multi; // this is for when i decide to add fragmenting the conn

	void parseIPv4(std::string ipv4) {
		int part = 0;

    	for (size_t i = 0; i < ipv4.size(); ++i) {
    	    char c = this->ipv4[i];
    	    if (c == '.') {
    	        ++part;
    	        ipv4[part] = 0;
    	    } else {
    	        ipv4[part] = ipv4[part] * 10 + (c - '0');
    	    }
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