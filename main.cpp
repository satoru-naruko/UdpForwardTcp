#include <iostream>
#include "UdpToTcpForwarder.h"

int main() {
    const std::string ip = {
        "127.0.0.1"
    };
    const unsigned short tcpPort = 54321;
    const unsigned short udpPort = 12345;

    UdpToTcpForwarder handler(ip, tcpPort, udpPort);
    if (!handler.initialize()) {
        std::cerr << "Failed to initialize TCPUDPHandler\n";
        return -1;
    }

    std::string command;
    while (true) {
        std::getline(std::cin, command);
        if (command == "exit") {
            break;
        }
    }

    handler.finalize();

    return 0;
}