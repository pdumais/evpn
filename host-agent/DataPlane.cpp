#include "DataPlane.h"
#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/route/neighbour.h>
#include <netlink/route/link.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

DataPlane::DataPlane()
{
    this->mBridge = "br0";
    this->mVxlanDevice = "vxlan1";
}

void DataPlane::add_vtep(const std::string &vtep_address)
{
    std::cout << "Accepting route advertisement: " << vtep_address << std::endl;

    struct nl_sock *sock = nl_socket_alloc();
    int err = nl_connect(sock, NETLINK_ROUTE);
    if (err < 0)
    {
        std::cout << "nl_connect: " << err << std::endl;
        nl_socket_free(sock);
        return;
    }

    struct nl_msg *msg = nlmsg_alloc();

    struct nlmsghdr *nlh = nlmsg_put(msg, 0, NL_AUTO_SEQ, RTM_NEWNEIGH, sizeof(struct ndmsg), NLM_F_REQUEST | NLM_F_CREATE | NLM_F_APPEND);
    struct ndmsg *ndm = (struct ndmsg *)nlmsg_data(nlh);
    ndm->ndm_family = AF_BRIDGE;
    ndm->ndm_ifindex = if_nametoindex(this->mVxlanDevice.c_str());
    ndm->ndm_state = NUD_PERMANENT | NUD_NOARP;
    ndm->ndm_flags = NTF_SELF;

    uint8_t mac_addr[6] = {0};
    nla_put(msg, NDA_LLADDR, sizeof(mac_addr), mac_addr);

    struct in_addr ip_addr;
    inet_pton(AF_INET, vtep_address.c_str(), &ip_addr);
    nla_put(msg, NDA_DST, sizeof(ip_addr), &ip_addr);

    nl_send_auto(sock, msg);

    nlmsg_free(msg);
    nl_socket_free(sock);
}

void DataPlane::remove_vtep(const std::string &vtep_address)
{
    // TODO: Delete prefix
    std::cout << "Deleting prefix: " << vtep_address << std::endl;

    struct nl_sock *sock = nl_socket_alloc();
    int err = nl_connect(sock, NETLINK_ROUTE);
    if (err < 0)
    {
        std::cout << "nl_connect: " << err << std::endl;
        nl_socket_free(sock);
        return;
    }

    struct nl_msg *msg = nlmsg_alloc();

    struct nlmsghdr *nlh = nlmsg_put(msg, 0, NL_AUTO_SEQ, RTM_DELNEIGH, sizeof(struct ndmsg), NLM_F_REQUEST | NLM_F_CREATE | NLM_F_APPEND);
    struct ndmsg *ndm = (struct ndmsg *)nlmsg_data(nlh);
    ndm->ndm_family = AF_BRIDGE;
    ndm->ndm_ifindex = if_nametoindex(this->mVxlanDevice.c_str());
    ndm->ndm_state = NUD_PERMANENT | NUD_NOARP;
    ndm->ndm_flags = NTF_SELF;

    uint8_t mac_addr[6] = {0};
    nla_put(msg, NDA_LLADDR, sizeof(mac_addr), mac_addr);

    struct in_addr ip_addr;
    inet_pton(AF_INET, vtep_address.c_str(), &ip_addr);
    nla_put(msg, NDA_DST, sizeof(ip_addr), &ip_addr);

    nl_send_auto(sock, msg);

    nlmsg_free(msg);
    nl_socket_free(sock);
}
