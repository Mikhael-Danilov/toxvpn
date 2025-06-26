#include "route.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>

int netlink_socket;

static struct {
    struct nlmsghdr nl;
    struct rtmsg rt;
    char buf[8192];
} req;

void send_request();

void route_init() {
    netlink_socket = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (netlink_socket < 0) {
        perror("Failed to create netlink socket");
    }
}

void systemRouteAdd(int ifindex, struct in_addr network, unsigned char prefixlen, const char* gateway) {
    struct rtattr* rtap;

    bzero(&req, sizeof(req));

    int rtl = sizeof(struct rtmsg);

    // Destination Attribute
    rtap = (struct rtattr*) req.buf;
    rtap->rta_type = RTA_DST;
    rtap->rta_len = sizeof(struct rtattr) + 4;
    memcpy(RTA_DATA(rtap), &network, 4);
    rtl += rtap->rta_len;

    // Gateway Attribute (if provided)
    if (gateway) {
        rtap = (struct rtattr*) (((char*) rtap) + rtap->rta_len);
        rtap->rta_type = RTA_GATEWAY;
        rtap->rta_len = sizeof(struct rtattr) + 4;
        inet_pton(AF_INET, gateway, RTA_DATA(rtap));
        rtl += rtap->rta_len;
    }

    // Outgoing Interface Attribute
    rtap = (struct rtattr*) (((char*) rtap) + rtap->rta_len);
    rtap->rta_type = RTA_OIF;
    rtap->rta_len = sizeof(struct rtattr) + 4;
    memcpy(RTA_DATA(rtap), &ifindex, 4);
    rtl += rtap->rta_len;

    // Netlink Header
    req.nl.nlmsg_len = NLMSG_LENGTH(rtl);
    req.nl.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
    req.nl.nlmsg_type = RTM_NEWROUTE;

    // Service Header
    req.rt.rtm_family = AF_INET;
    req.rt.rtm_table = RT_TABLE_MAIN;
    req.rt.rtm_protocol = RTPROT_STATIC;
    req.rt.rtm_scope = RT_SCOPE_UNIVERSE;
    req.rt.rtm_type = RTN_UNICAST;
    req.rt.rtm_dst_len = prefixlen;

    send_request();
}

void systemRouteSingle(int ifindex, struct in_addr peer, const char* gateway) {
    systemRouteAdd(ifindex, peer, 32, gateway);
}

void send_request() {
    struct sockaddr_nl pa;
    bzero(&pa, sizeof(pa));
    pa.nl_family = AF_NETLINK;

    struct msghdr msg;
    bzero(&msg, sizeof(msg));
    msg.msg_name = &pa;
    msg.msg_namelen = sizeof(pa);

    struct iovec iov;
    iov.iov_base = (void*) &req.nl;
    iov.iov_len = req.nl.nlmsg_len;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ssize_t res = sendmsg(netlink_socket, &msg, 0);
    if(res < 0) {
        printf("route error: %s\n", strerror(errno));
    }
}