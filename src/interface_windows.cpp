#include "main.h"

using namespace std;
using namespace ToxVPN;

NetworkInterface::NetworkInterface() : fd(0), my_tox(nullptr), verbose(false) { }
void NetworkInterface::configure(string ip_in, Tox* tox_in, string tunDevice) { my_tox = tox_in; }
