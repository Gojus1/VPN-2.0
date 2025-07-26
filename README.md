<h1 align="center">VPN 2.0</h1>

<p align="center">
    Lightweight multithreaded proxy over TCP<br>
    <i>Pure C++ implementation. Built for speed and minimalism.</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C++17-std-blue.svg">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey">
</p>

---

## Overview

VPN 2.0 is a TCP-level proxy that mimics basic VPN behavior through simple forwarding.  
The client connects to the proxy server and requests a target destination. The server resolves the destination and forwards traffic accordingly.

No encryption. No external dependencies. Just sockets and threads.

---

## Features

- Multithreaded client handling
- IP + port forwarding with destination parsing
- Platform support for **Windows** & **Linux**
- Simple HTTP tunneling (default: `httpbin.org`)
- Flag structure for chaining/multi-routing (planned)

---

## Usage

### Build

#### Windows (MSVC)

```sh
cl main.cpp vpn_client.cpp /I util ws2_32.lib
cl server.cpp /I util ws2_32.lib
```

#### Linux (g++)

```sh
g++ main.cpp vpn_client.cpp -o client -pthread
g++ server.cpp -o server -pthread
```

> Requires C++17 or later

---

### Run

#### Start the server:
```sh
./server
```

#### Connect with client:
```sh
./client -ipv4 127.0.0.1 -p 5000
```

---

## Flags

| Flag     | Description                     |
|----------|---------------------------------|
| `-ipv4`  | Proxy server IP (default: 127.0.0.1) |
| `-p`     | Port to connect to (default: 5000) |
| `-pass`  | Optional password (unused for now) |
| `-multi` | Enables multi-routing (WIP)     |

---

## Example

```sh
./client -ipv4 127.0.0.1 -p 5000
```

Prints your public IP by routing a `GET /ip` request through the proxy.

---

## Notes

- Server listens on port 5000 by default
- Client requests are parsed line-by-line: `host:port` followed by HTTP payload
- Modular design with `Flags`, `Client`, and server logic split

---

## License

MIT — do whatever. No warranty. Use with caution.

---

## Authors

> [Gojus1](https://github.com/Gojus1) and [GreyClouds](https://github.com/greycloudss)
