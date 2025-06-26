#include "main.h" // Assuming main.h contains common headers like ifreq, ioctl, etc.
#include "interface.h"

#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <net/if.h>


using namespace std;
using namespace ToxVPN;

// This is the function that will be run in a new thread to handle reading from the interface.
static void* start_routine(void* x) {
    NetworkInterface* nic = (NetworkInterface*) x;
    return nic->loop();
}

// Constructor: Opens the /dev/net/tun file descriptor.
NetworkInterface::NetworkInterface() : my_tox(nullptr), isGatewaySet(false) { // Added isGatewaySet initialization
    fd = 0;
    if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        cerr << "FATAL: unable to open /dev/net/tun. Check permissions or if the 'tun' module is loaded." << endl;
        // In a real application, you might throw an exception here.
        exit(-1);
    }
}

void NetworkInterface::configure(string ip_in, Tox* tox_in) {
    int err;
    // The local 'struct ifreq ifr;' has been removed. We now use the class member 'this->ifr'.
    memset(&this->ifr, 0, sizeof(this->ifr));

    this->ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(this->ifr.ifr_name, "tox_master%d", IFNAMSIZ);

    if((err = ioctl(fd, TUNSETIFF, (void*) &this->ifr)) < 0) {
        if(errno == EPERM) {
            cerr << "FATAL: No permission to create a TUN device. Please run as root." << endl;
            exit(-1);
        }
        cerr << "FATAL: Error creating TUN device: " << strerror(errno) << " (" << err << ")" << endl;
        close(fd);
        exit(-1);
    }

    cout << "Created TUN device: " << this->ifr.ifr_name << endl;

    int tun_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(tun_sock < 0) {
        cerr << "FATAL: Could not create socket for interface configuration: " << strerror(errno) << endl;
        return;
    }

    this->ifr.ifr_mtu = 1200;
    if((err = ioctl(tun_sock, SIOCSIFMTU, &this->ifr))) {
        printf("Warning: Error %d setting MTU: %s\n", err, strerror(errno));
    }

    printf("Setting interface IP to %s\n", ip_in.c_str());
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_aton(ip_in.c_str(), &address.sin_addr);
    memcpy(&this->ifr.ifr_addr, &address, sizeof(address));
    if((err = ioctl(tun_sock, SIOCSIFADDR, &this->ifr))) {
        printf("Warning: Error %d setting IP address: %s\n", errno, strerror(errno));
    }

    inet_aton("10.123.123.123", &address.sin_addr);
    memcpy(&this->ifr.ifr_dstaddr, &address, sizeof(address));
    if((err = ioctl(tun_sock, SIOCSIFDSTADDR, &this->ifr))) {
        printf("Warning: Error setting destination IP: %s\n", strerror(errno));
    }

    this->ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    ioctl(tun_sock, SIOCSIFFLAGS, &this->ifr);

    close(tun_sock);

    interfaceIndex = if_nametoindex(this->ifr.ifr_name);
    my_tox = tox_in;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&reader, &attr, &start_routine, this);
    pthread_attr_destroy(&attr);
}