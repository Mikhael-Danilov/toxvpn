#pragma once

#include <netinet/in.h>

void route_init();
void systemRouteSingle(int ifindex, struct in_addr peer, const char* gateway);
void systemRouteAdd(int ifindex, struct in_addr network, unsigned char prefixlen, const char* gateway);