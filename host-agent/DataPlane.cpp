#include "DataPlane.h"
#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/route/neighbour.h>
#include <netlink/route/link.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sstream>

DataPlane::DataPlane()
{
    this->mBridge = "br0";
    this->mVxlanDevice = "vxlan1";
}
bool DataPlane::mac_string_to_bytes(const std::string &mac, uint8_t *buf)
{
    int index = 0;

    std::istringstream iss(mac);
    std::string tmp;
    while (std::getline(iss, tmp, ':'))
    {
        buf[index] = static_cast<uint8_t>(std::stoi(tmp, 0, 16));
        index++;
        if (index >= 6)
        {
            return false;
        }
    }

    return true;
}

void DataPlane::add_mac(const std::string &mac, const std::string &vtep_address, int vni)
{
    std::cout << "Accepting route advertisement: " << mac << ":" << vtep_address << ":" << vni << std::endl;

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

    uint8_t mac_addr[6];
    this->mac_string_to_bytes(mac, mac_addr);

    nla_put(msg, NDA_LLADDR, sizeof(mac_addr), mac_addr);
    nla_put_u32(msg, NDA_SRC_VNI, vni);

    struct in_addr ip_addr;
    inet_pton(AF_INET, vtep_address.c_str(), &ip_addr);
    nla_put(msg, NDA_DST, sizeof(ip_addr), &ip_addr);

    nl_send_auto(sock, msg);

    nlmsg_free(msg);
    nl_socket_free(sock);
}

void DataPlane::add_vtep(const std::string &vtep_address, int vni)
{
    std::cout << "Accepting route advertisement: " << vtep_address << ":" << vni << std::endl;

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

    uint8_t mac_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    nla_put(msg, NDA_LLADDR, sizeof(mac_addr), mac_addr);
    nla_put_u32(msg, NDA_SRC_VNI, vni);

    struct in_addr ip_addr;
    inet_pton(AF_INET, vtep_address.c_str(), &ip_addr);
    nla_put(msg, NDA_DST, sizeof(ip_addr), &ip_addr);

    nl_send_auto(sock, msg);

    nlmsg_free(msg);
    nl_socket_free(sock);
}

void DataPlane::remove_vtep(const std::string &vtep_address, int vni)
{
    std::cout << "Deleting vtep: " << vtep_address << ":" << vni << std::endl;

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
    nla_put_u32(msg, NDA_SRC_VNI, vni);

    nl_send_auto(sock, msg);

    nlmsg_free(msg);
    nl_socket_free(sock);
}

void DataPlane::remove_mac(const std::string &mac, const std::string &vtep_address, int vni)
{
    std::cout << "Deleting mac: " << mac << ":" << vtep_address << ":" << vni << std::endl;

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

    uint8_t mac_addr[6];
    this->mac_string_to_bytes(mac, mac_addr);
    nla_put(msg, NDA_LLADDR, sizeof(mac_addr), mac_addr);

    struct in_addr ip_addr;
    inet_pton(AF_INET, vtep_address.c_str(), &ip_addr);
    nla_put(msg, NDA_DST, sizeof(ip_addr), &ip_addr);
    nla_put_u32(msg, NDA_SRC_VNI, vni);

    err = nl_send_auto(sock, msg);
    if (err < 0)
    {
        std::cout << "nl_send_auto: " << err << std::endl;
    }

    nlmsg_free(msg);
    nl_socket_free(sock);
}
