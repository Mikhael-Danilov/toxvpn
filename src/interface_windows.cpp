#include "main.h"

using namespace std;
using namespace ToxVPN;

NetworkInterface::NetworkInterface() { fd = 0; }
void NetworkInterface::configure(string ip_in, Tox* tox_in, string masquerade_iface) { 
    my_tox = tox_in; 
    // Windows NAT configuration would require additional implementation
    if(!masquerade_iface.empty()) {
        printf("masquerade mode not yet implemented on Windows\n");
    }
}
