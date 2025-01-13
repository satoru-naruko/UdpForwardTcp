#ifndef TCP_UDP_HANDLER_H
#define TCP_UDP_HANDLER_H

#pragma once

#include <string>
#include <thread>
#include <unistd.h>
#include <queue>
#include <mutex>
#include <condition_variable>

class UdpToTcpForwarder
{
public:
    UdpToTcpForwarder(const std::string &ip, unsigned short tcpPort, unsigned short udpPort)
        : ip_(ip), tcpPort_(tcpPort), udpPort_(udpPort) /*, tcpSockfd_(-1)*/ {}
    ~UdpToTcpForwarder() {}

    bool initialize();
    void finalize();

private:
    // int establishTCPConnection(const std::string& ip, unsigned short tcpPort);
    void handleTCPConnection();
    void receiveUDPAndForward();
    void enqueueTcpSendMessage(const std::string &msg);
    void sendLoop();
    void join();

    bool running_ = true;
    std::string ip_;
    unsigned short tcpPort_;
    unsigned short udpPort_;
    // int tcpSockfd_;
    // std::thread recvThread_;
    std::thread udpThread_;

    std::queue<std::string> messageQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCondVar_;
    std::thread sendThread_;
};

#endif // TCP_UDP_HANDLER_H