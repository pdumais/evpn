#!/bin/bash
set -x
set -e

function find_next_free_vlan()
{
    VNI=$1
    PNS=$2
    # Since we can have 16M VNIs but only 4096 vlans, then VNIs will not necessarily map 1:1 to vlans.
    # We will first check if a vlan mapping exists on this hypervisor. If it doesnt, then we will create
    # a new mapping from a sequentially assigned vlan number. The same VNI may not be mapped the same way 
    # on all hypervisors.
    # List the vlans on the interface and get the highest number, then return that number +1.
    # If the vlan already exist for that VNI, then we will return that.
    
    VLAN=$(ip netns exec $PNS bridge -d -j vlan tunnelshow dev vxlan1 | python3 vni2vlan.py ${VNI})
    echo ${VLAN}
}

PARENT_NS_NUM=$1
SUBNUM=$2
IP=$3
VNI=$4
PARENT_NS=evpn-${PARENT_NS_NUM}
VLAN=$(find_next_free_vlan ${VNI} ${PARENT_NS})
SUBNS=evpn-e${PARENT_NS_NUM}${SUBNUM}

ip netns exec ${PARENT_NS} ip link add veth1-${SUBNUM} type veth peer name veth2-${SUBNUM}
ip netns exec ${PARENT_NS} ip link set veth1-${SUBNUM} master br0
ip netns exec ${PARENT_NS} ip link set veth1-${SUBNUM} up
ip netns add ${SUBNS}
ip netns exec ${SUBNS} ip link set lo up
ip netns exec ${PARENT_NS} ip link set veth2-${SUBNUM} netns ${SUBNS}
ip netns exec ${SUBNS} ip link set veth2-${SUBNUM} name eth0
ip netns exec ${SUBNS} ip link set eth0 up
ip netns exec ${SUBNS} ip a a ${IP} dev eth0

# We now need to set the vld on the veth
ip netns exec ${PARENT_NS} bridge vlan add vid ${VLAN} pvid untagged dev veth1-${SUBNUM}
ip netns exec ${PARENT_NS} bridge vlan del vid 1 dev veth1-${SUBNUM}

# We need to add tunnel info on the vxlan interface
ip netns exec ${PARENT_NS} bridge vlan add dev vxlan1 vid ${VLAN}
ip netns exec ${PARENT_NS} bridge vlan add dev vxlan1 vid  ${VLAN} tunnel_info id ${VNI} || true

# Announce the vtep and VNI. I am setting the etag to the vlan number. Not sure if this is allowed. I'm doing this only for being able to
# track this information.
ip netns exec ${PARENT_NS} ./gobgp global rib -a evpn add multicast 10.0.0.$((100 + PARENT_NS_NUM)) etag ${VLAN} rd 10.0.0.$((100 + PARENT_NS_NUM)):${VNI} encap vxlan pmsi ingress-repl ${VNI} 10.0.0.$((100 + PARENT_NS_NUM))


# Announce the MAC address
MAC_ADDRESS=$(ip netns exec ${SUBNS} cat /sys/class/net/eth0/address)
echo $MAC_ADDRESS
ip netns exec ${PARENT_NS} ./gobgp global rib -a evpn add macadv ${MAC_ADDRESS} 0.0.0.0 etag 0 label ${VNI} rd 10.0.0.$((100 + PARENT_NS_NUM)):${VNI} encap vxlan pmsi ingress-repl ${VNI} 10.0.0.$((100 + PARENT_NS_NUM))