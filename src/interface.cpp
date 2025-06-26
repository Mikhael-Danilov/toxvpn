#include <algorithm>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <utility> // For std::pair

#include "interface.h"
#include "route.h"

using namespace std;
using namespace ToxVPN;

// --- Helper Functions ---
static string exec_command(const char* cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

static bool run_command(const string& cmd) {
    cout << "Executing: " << cmd << endl;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        cerr << "FATAL: Command failed. Aborting." << endl;
        return false;
    }
    return true;
}
static pair<string, string> find_default_route_info() {
    ifstream route_file("/proc/net/route");
    if (!route_file.is_open()) return {"", ""};
    string line;
    while (getline(route_file, line)) {
        istringstream iss(line);
        string iface, destination, gateway_hex, flags;
        iss >> iface >> destination >> gateway_hex >> flags;
        if (destination == "00000000" && (stoi(flags, nullptr, 16) & 0x2)) {
            struct in_addr gateway_addr;
            gateway_addr.s_addr = stoul(gateway_hex, nullptr, 16);
            return {iface, inet_ntoa(gateway_addr)};
        }
    }
    return {"", ""};
}

bool compareRoutes(const Route& a, const Route& b) {
    return a.maskbits > b.maskbits;
}
// --- NetworkInterface Implementation ---

// THIS IS THE FIX: The constructor definition that was here has been REMOVED.
// The one and only definition is now in interface_linux.cpp, as it should be.

NetworkInterface::~NetworkInterface() {
    if (isGatewaySet) {
        cleanupGatewayRoutes();
    }
}

void NetworkInterface::sortRoutes() {
    routes.sort(compareRoutes);
}

void NetworkInterface::setInternetGateway(int friend_number) {
    if (isGatewaySet) {
        cout << "Gateway is already set." << endl;
        return;
    }
    cout << "Setting friend #" << friend_number << " as the internet gateway..." << endl;

    const char* tun_gateway = "10.123.123.123";
    const string tun_name = string(this->ifr.ifr_name);
    const string vpn_network = "192.168.55.0/24"; // Your VPN's IP range

    if (!run_command("ip route add default dev " + tun_name + " via " + tun_gateway + " table 101")) return;
    if (!run_command("ip rule add fwmark 2 lookup 101")) return;

    // --- THE FINAL, CORRECT FIX ---

    // Rule 1 (High Priority): This is the loop prevention.
    // It says: "For any packet on the OUTPUT chain, if its destination is another
    // computer on our VPN network, do NOT process any more marking rules. Let it go."
    // We use the special target 'RETURN' which means stop processing this chain.
    if (!run_command("iptables -t mangle -I OUTPUT 1 -d " + vpn_network + " -j RETURN")) {
        cleanupGatewayRoutes();
        return;
    }

    // Rule 2 (Lower Priority): This is our main marking rule.
    // It will only be reached by packets NOT destined for the VPN network.
    // It says: "Mark any remaining packets from non-root users for redirection."
    if (!run_command("iptables -t mangle -A OUTPUT -m owner ! --uid-owner 0 -j MARK --set-mark 2")) {
        cleanupGatewayRoutes();
        return;
    }

    Route default_route;
    inet_pton(AF_INET, "0.0.0.0", &default_route.network);
    inet_pton(AF_INET, "0.0.0.0", &default_route.mask);
    default_route.maskbits = 0;
    default_route.friend_number = friend_number;
    routes.push_back(default_route);
    sortRoutes();

    isGatewaySet = true;
    cout << "Internet gateway configured successfully." << endl;
}

void NetworkInterface::cleanupGatewayRoutes() {
    cout << "Cleaning up gateway routes and rules..." << endl;
    const string vpn_network = "192.168.55.0/24";

    // Clean up in reverse order of creation
    run_command("iptables -t mangle -D OUTPUT -m owner ! --uid-owner 0 -j MARK --set-mark 2");
    run_command("iptables -t mangle -D OUTPUT -d " + vpn_network + " -j RETURN");

    run_command("ip rule del fwmark 2 lookup 101");
    run_command("ip route flush table 101");
    cout << "Cleanup complete." << endl;
}


// Finds the best route for a peer using longest prefix matching.
bool NetworkInterface::findRoute(Route* route, struct in_addr peer) {
    for(const auto& r : routes) {
        if((peer.s_addr & r.mask.s_addr) == (r.network.s_addr & r.mask.s_addr)) {
            *route = r;
            return true;
        }
    }
    return false;
}

void NetworkInterface::addPeerRoute(struct in_addr peer, int friend_number) {
    Route x;
    x.network = peer;
    inet_pton(AF_INET, "255.255.255.255", &x.mask);
    x.maskbits = 32;
    x.friend_number = friend_number;
    routes.push_back(x);
    sortRoutes();
    systemRouteSingle(interfaceIndex, peer, "10.123.123.123");
}

// ... the rest of the file is unchanged ...
void* NetworkInterface::loop() {
    fd_set readset;
    struct timeval timeout;
    int r;
    while(true) {
        FD_ZERO(&readset);
        FD_SET(fd, &readset);
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;
        r = select(fd + 1, &readset, nullptr, nullptr, &timeout);
        if(r > 0) {
            if(FD_ISSET(fd, &readset))
                handleReadData();
        } else if(r < 0) {
            printf("select error fd:%d r:%d errno:%d %s\n", fd, r, errno, strerror(errno));
        }
    }
    return nullptr;
}

void NetworkInterface::handleReadData() {
    uint8_t readbuffer[1500];
    ssize_t size = read(fd, readbuffer, 1500);
    if(size < 0) {
        printf("unable to read from tun %d, %s\n", fd, strerror(errno));
        return;
    }

#ifdef __APPLE__
#define OFFSET 5
    struct in_addr* dest = (struct in_addr*) (readbuffer + 16);
#else
#define OFFSET 1
    struct in_addr* dest = (struct in_addr*) (readbuffer + 20);
#endif

    Route route;
    if(findRoute(&route, *dest)) {
        forwardPacket(route, readbuffer, size);
    } else {
        printf("no route found for %s\n", inet_ntoa(*dest));
    }
}

void NetworkInterface::forwardPacket(Route route, uint8_t* readbuffer, ssize_t size) {
    uint8_t buffer[1500 + OFFSET];
    buffer[0] = 200;
#ifdef __APPLE__
    buffer[1] = 0; buffer[2] = 0; buffer[3] = 0x08; buffer[4] = 0;
#endif
    memcpy(buffer + OFFSET, readbuffer, size);
    Tox_Err_Friend_Custom_Packet error;
    tox_friend_send_lossy_packet(my_tox, route.friend_number, buffer, size + OFFSET, &error);
    if (error != TOX_ERR_FRIEND_CUSTOM_PACKET_OK) {
        // Error handling can be improved
    }
}

void NetworkInterface::setPeerIp(struct in_addr peer, int friend_number) {
    addPeerRoute(peer, friend_number);
}

void NetworkInterface::removePeer(int friend_number) {
    // This function should also be implemented to remove routes
}

void NetworkInterface::processPacket(const uint8_t* data, size_t size, int friend_number) {
    ssize_t ret = 0;
    if(fd > 0) {
#ifdef __APPLE__
        ret = write(fd, data + 4, size);
#else
        ret = write(fd, data, size);
#endif
    }
    if((size_t)ret != size)
        cerr << "partial packet write to tun\n";
}