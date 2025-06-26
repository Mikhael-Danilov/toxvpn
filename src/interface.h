#pragma once

#include <list>
#include <string>
#include <arpa/inet.h>
#include "tox/tox.h"
#include <linux/if.h> // <-- ADD THIS INCLUDE

namespace ToxVPN {

class Route {
public:
    struct in_addr network;
    struct in_addr mask;
    int maskbits;
    int friend_number;
};

class NetworkInterface {
public:
    NetworkInterface();
    ~NetworkInterface();
    void* loop();
    void setPeerIp(struct in_addr peer, int friend_number);
    void removePeer(int friend_number);
    void addPeerRoute(struct in_addr peer, int friend_number);
    void setInternetGateway(int friend_number);
    void processPacket(const uint8_t* data, size_t bytes, int friend_number);
    void configure(std::string myip, Tox* my_tox);

    std::list<Route> routes;

private:
    void handleReadData();
    bool findRoute(Route* route, struct in_addr peer);
    void forwardPacket(Route route, uint8_t* buffer, ssize_t bytes);
    void sortRoutes();
    void cleanupGatewayRoutes();

    pthread_t reader;
    int fd;
    Tox* my_tox;
    int interfaceIndex;
    bool isGatewaySet;
    struct ifreq ifr; // <-- ADD THIS CLASS MEMBER
};
}