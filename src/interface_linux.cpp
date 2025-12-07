#include "main.h"
#include "interface.h"

using namespace std;
using namespace ToxVPN;

static void* start_routine(void* x) {
    NetworkInterface* nic = (NetworkInterface*) x;
    return nic->loop();
}

NetworkInterface::NetworkInterface() : my_tox(nullptr), verbose(false) {
    fd = 0;
    // Don't open TUN device here - let configure() method handle it based on tunDevice parameter
}

void NetworkInterface::configure(string ip_in, Tox* tox_in, string tunDevice) {
    int err;

    // Open TUN device in both cases - we'll attach to existing or create new
    if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        cerr << "unable to open /dev/net/tun" << endl;
        exit(-1);
    }

    if (!tunDevice.empty()) {
        // Use pre-created TUN device
        cout << "Using pre-created TUN device: " << tunDevice << endl;

        // Configure to attach to existing interface
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TUN;
        strncpy(ifr.ifr_name, tunDevice.c_str(), IFNAMSIZ - 1);

        if((err = ioctl(fd, TUNSETIFF, (void*) &ifr)) < 0) {
            if(errno == EPERM) {
                cerr << "no permission to set existing tun device: " << tunDevice << endl;
                exit(-1);
            }
            cerr << "error attaching to existing tun device: " << strerror(errno) << err << endl;
            close(fd);
            exit(-1);
        }

        // Get interface index by name
        interfaceIndex = if_nametoindex(tunDevice.c_str());
        if (interfaceIndex == 0) {
            cerr << "unable to get interface index for: " << tunDevice << endl;
            exit(-1);
        }
    } else {
        // Create new TUN device (original behavior)
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
        interfaceIndex = if_nametoindex(ifr.ifr_name);
    }

    // Configure the interface (same for both cases)
    // Set MTU params
    int tun_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(tun_sock < 0) {
        printf("error while setting MTU: %s", strerror(errno));
        return;
    }

    // Set MTU if not using pre-created device (user should set MTU when creating)
    if (tunDevice.empty()) {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        if (!tunDevice.empty()) {
            strncpy(ifr.ifr_name, tunDevice.c_str(), IFNAMSIZ - 1);
        } else {
            strncpy(ifr.ifr_name, "tox_master%d", IFNAMSIZ - 1);
        }

        ifr.ifr_mtu = 1200;
        err = ioctl(tun_sock, SIOCSIFMTU, &ifr);
        if(err)
            printf("error %d setting mtu\n", err);
    }

    printf("setting ip to %s\n", ip_in.c_str());
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_aton(ip_in.c_str(), &address.sin_addr);
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    if (!tunDevice.empty()) {
        strncpy(ifr.ifr_name, tunDevice.c_str(), IFNAMSIZ - 1);
    } else {
        strncpy(ifr.ifr_name, "tox_master%d", IFNAMSIZ - 1);
    }

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

    close(tun_sock);

    my_tox = tox_in;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&reader, &attr, &start_routine, this);
    pthread_attr_destroy(&attr);
}
