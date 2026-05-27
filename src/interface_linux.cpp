#include "main.h"
#include "interface.h"

using namespace std;
using namespace ToxVPN;

static void* start_routine(void* x) {
    NetworkInterface* nic = (NetworkInterface*) x;
    return nic->loop();
}

NetworkInterface::NetworkInterface() : my_tox(nullptr) {
    fd = 0;
    if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        cerr << "unable to open /dev/net/tun" << endl;
    }
}

void NetworkInterface::configure(string ip_in, Tox* tox_in, string masquerade_iface) {
    int err;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    strncpy(ifr.ifr_name, "tox_master%d", IFNAMSIZ);

    if((err = ioctl(fd, TUNSETIFF, (void*) &ifr)) < 0) {
        if(errno == EPERM) {
            cerr << "no permission to create tun device" << endl;
            exit(-1);
        }
        cerr << strerror(errno) << err << endl;
        close(fd);
    }
    // and set MTU params
    int tun_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(tun_sock < 0) {
        printf("error while setting MTU: %s", strerror(errno));
        return;
    }
    ifr.ifr_mtu = 1200;
    err = ioctl(tun_sock, SIOCSIFMTU, &ifr);
    if(err)
        printf("error %d setting mtu\n", err);

    printf("setting ip to %s\n", ip_in.c_str());
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_aton(ip_in.c_str(), &address.sin_addr);
    memcpy(&ifr.ifr_addr, &address, sizeof(address));
    err = ioctl(tun_sock, SIOCSIFADDR, &ifr);
    if(err)
        printf("error %d %s setting ip\n", errno, strerror(errno));

    inet_aton("10.123.123.123", &address.sin_addr);
    memcpy(&ifr.ifr_dstaddr, &address, sizeof(address));
    err = ioctl(tun_sock, SIOCSIFDSTADDR, &ifr);
    if(err)
        printf("error setting dest ip: %s\n", strerror(errno));

    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    ioctl(tun_sock, SIOCSIFFLAGS, &ifr);

    // Setup masquerading if interface specified
    if(!masquerade_iface.empty()) {
        printf("setting up masquerade from %s to %s\n", ifr.ifr_name, masquerade_iface.c_str());
        
        // First, try to clean up any leftover rules from previous unclean shutdowns
        char cleanup_cmd[512];
        snprintf(cleanup_cmd, sizeof(cleanup_cmd), "iptables -t nat -D POSTROUTING -s 10.123.123.0/24 -o %s -j MASQUERADE 2>/dev/null", masquerade_iface.c_str());
        system(cleanup_cmd);
        
        // Enable IP forwarding
        system("echo 1 > /proc/sys/net/ipv4/ip_forward");
        
        // Setup iptables MASQUERADE rule
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "iptables -t nat -A POSTROUTING -s 10.123.123.0/24 -o %s -j MASQUERADE", masquerade_iface.c_str());
        int ret = system(cmd);
        if(ret != 0) {
            printf("warning: iptables MASQUERADE rule failed (exit code %d)\n", ret);
        }
        
        // Allow forwarding
        snprintf(cmd, sizeof(cmd), "iptables -A FORWARD -i %s -o %s -j ACCEPT", ifr.ifr_name, masquerade_iface.c_str());
        ret = system(cmd);
        if(ret != 0) {
            printf("warning: iptables FORWARD rule failed (exit code %d)\n", ret);
        }
        
        snprintf(cmd, sizeof(cmd), "iptables -A FORWARD -i %s -o %s -m state --state RELATED,ESTABLISHED -j ACCEPT", masquerade_iface.c_str(), ifr.ifr_name);
        ret = system(cmd);
        if(ret != 0) {
            printf("warning: iptables FORWARD established rule failed (exit code %d)\n", ret);
        }
        
        // Store interface info in instance for cleanup
        tun_interface = ifr.ifr_name;
        masq_interface = masquerade_iface;
    }

    close(tun_sock);

    interfaceIndex = if_nametoindex(ifr.ifr_name);
    my_tox = tox_in;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&reader, &attr, &start_routine, this);
    pthread_attr_destroy(&attr);
}

NetworkInterface::~NetworkInterface() {
    // Cleanup masquerade rules if they were set up
    if(!masq_interface.empty() && !tun_interface.empty()) {
        printf("cleaning up masquerade rules for %s\n", tun_interface.c_str());
        
        char cmd[512];
        
        // Remove iptables MASQUERADE rule
        snprintf(cmd, sizeof(cmd), "iptables -t nat -D POSTROUTING -s 10.123.123.0/24 -o %s -j MASQUERADE", masq_interface.c_str());
        system(cmd);
        
        // Remove FORWARD rules
        snprintf(cmd, sizeof(cmd), "iptables -D FORWARD -i %s -o %s -j ACCEPT", tun_interface.c_str(), masq_interface.c_str());
        system(cmd);
        
        snprintf(cmd, sizeof(cmd), "iptables -D FORWARD -i %s -o %s -m state --state RELATED,ESTABLISHED -j ACCEPT", masq_interface.c_str(), tun_interface.c_str());
        system(cmd);
    }
    
    // Close the TUN device
    if(fd > 0) {
        close(fd);
    }
}
