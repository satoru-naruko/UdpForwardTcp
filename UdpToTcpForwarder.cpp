#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "UdpToTcpForwarder.h"
#include "UdpToTcpForwarderLog.h"

bool UdpToTcpForwarder::initialize() {
    
    UDP_TO_TCP_FORWARDER_LOG("initialize start. \n");
    
    running_ = true;

    // int ret = establishTCPConnection(ip_, tcpPort_);
    // if (ret < 0) {
    //     return false;
    // }
    //recvThread_ = std::thread(&TCPUDPHandler::handleTCPConnection, this);
    udpThread_ = std::thread(&UdpToTcpForwarder::receiveUDPAndForward, this);
    sendThread_ = std::thread(&UdpToTcpForwarder::sendLoop, this);

    UDP_TO_TCP_FORWARDER_LOG("initialize end. \n");

    return true;
}

void UdpToTcpForwarder::finalize() {
    
    UDP_TO_TCP_FORWARDER_LOG("finalize start. \n");

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        running_ = false;
    }

    UDP_TO_TCP_FORWARDER_LOG("finalize notify start. \n");
    queueCondVar_.notify_one();
    
    join();

    UDP_TO_TCP_FORWARDER_LOG("finalize end. \n");
}

void UdpToTcpForwarder::join() {

    UDP_TO_TCP_FORWARDER_LOG("join start. \n");

    udpThread_.join();
    UDP_TO_TCP_FORWARDER_LOG("udpThread_ join success.\n");

    sendThread_.join();
    UDP_TO_TCP_FORWARDER_LOG("sendThread_ join success.\n");
    
    //recvThread_.join();
    //close(tcpSockfd_);
    UDP_TO_TCP_FORWARDER_LOG("join end. \n");
}

// int TCPUDPHandler::establishTCPConnection(const std::string& ip, unsigned short tcpPort)
// {
//     tcpSockfd_ = socket(AF_INET, SOCK_STREAM, 0);
//     if (tcpSockfd_ < 0) {
//         std::cerr << "TCP socket creation failed\n";
//         return -1;
//     }

//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(tcpPort);
//     inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

//     if (connect(tcpSockfd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
//         std::cerr << "TCP connect failed\n";
//         close(tcpSockfd_);
//         this->tcpSockfd_ = -1;
//         return -1;
//     }

//     std::cout << "establishTCPConnection success" << std::endl;

//     return 0;
// }

// void TCPUDPHandler::handleTCPConnection() {

//     std::cout << "handleTCPConnection" << std::endl;

//     char buffer[2048] = {0};
//     while (this->running_) {
//         ssize_t len = recv(tcpSockfd_, buffer, sizeof(buffer) - 1, 0);
//         if (len > 0) {
//             buffer[len] = '\0';
//             std::cout << "[TCP] Received: " << buffer << std::endl;
//         } else if (len == 0) {
//             std::cout << "[TCP] Connection closed by peer." << std::endl;
//             break;
//         } else {
//             std::cerr << "[TCP] recv error: " << strerror(errno) << std::endl;
//             break;
//         }
//     }
//     close(tcpSockfd_);
//     this->tcpSockfd_ = -1;
// }

void UdpToTcpForwarder::receiveUDPAndForward() {

    UDP_TO_TCP_FORWARDER_LOG("receiveUDPAndForward start. \n");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "UDP socket creation failed\n";
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(udpPort_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "UDP bind failed\n";
        close(sockfd);
        return;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (running_) {
        char buffer[2048] = {0};
        sockaddr_in sender{};
        socklen_t senderLen = sizeof(sender);
        ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                               (sockaddr*)&sender, &senderLen);
        if (len > 0) {
            buffer[len] = '\0';

            UDP_TO_TCP_FORWARDER_LOG("[UDP] Received: %s\n", buffer);
            enqueueTcpSendMessage(buffer);
        } else if (len < 0) {
            UDP_TO_TCP_FORWARDER_LOG("[UDP] recvfrom : %s\n", strerror(errno));
        }
    }
  
    close(sockfd);
    UDP_TO_TCP_FORWARDER_LOG("receiveUDPAndForward end. \n");
}

void UdpToTcpForwarder::enqueueTcpSendMessage(const std::string& msg) {
    UDP_TO_TCP_FORWARDER_LOG("enqueueTcpSendMessage start.\n");

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        messageQueue_.push(msg);
    }
    queueCondVar_.notify_one();

    UDP_TO_TCP_FORWARDER_LOG("enqueueTcpSendMessage end.\n");
}

void UdpToTcpForwarder::sendLoop() {
    UDP_TO_TCP_FORWARDER_LOG("sendLoop start, running_=%s\n", running_ ? "true" : "false");
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        queueCondVar_.wait(lock, [this]() {
            return !running_ || !messageQueue_.empty();
        });

        UDP_TO_TCP_FORWARDER_LOG("sendLoop exit waiting\n");

        if (running_ == false) {
            lock.unlock();
            std::cout << "sendLoop finish loop" << std::endl;
            break;
        }

        std::string msg = messageQueue_.front();
        messageQueue_.pop();
        lock.unlock();

        int tcpSockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (tcpSockfd < 0) {
            std::cerr << "TCP socket creation failed\n";
            return;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(tcpPort_);
        inet_pton(AF_INET, this->ip_.c_str(), &addr.sin_addr);

        if (connect(tcpSockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "TCP connect failed\n";
            close(tcpSockfd);
            tcpSockfd = -1;
            continue;
        }

        UDP_TO_TCP_FORWARDER_LOG("connect success. fd=%d msg=%s\n", tcpSockfd, msg.c_str());
        int sentByte = send(tcpSockfd, msg.c_str(), msg.size(), 0);

        UDP_TO_TCP_FORWARDER_LOG("send success. sentByte=%d\n", sentByte);
        close(tcpSockfd);
        tcpSockfd = -1;
    }

    UDP_TO_TCP_FORWARDER_LOG("sendLoop end. \n");
}
