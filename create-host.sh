#!/bin/bash
set -x
set -e

NUM=$1
NS=evpn-${NUM}

function create_subns() {
    SUBNUM=$1
    SUBNS=evpn-e${NUM}${SUBNUM}
    ip netns exec ${NS} ip link add veth1-${SUBNUM} type veth peer name veth2-${SUBNUM}
    ip netns exec ${NS} ip link set veth1-${SUBNUM} master br0
    ip netns exec ${NS} ip link set veth1-${SUBNUM} up
    ip netns add ${SUBNS}
    ip netns exec ${SUBNS} ip link set lo up
    ip netns exec ${NS} ip link set veth2-${SUBNUM} netns ${SUBNS}
    ip netns exec ${SUBNS} ip link set veth2-${SUBNUM} name eth0
    ip netns exec ${SUBNS} ip link set eth0 up
    ip netns exec ${SUBNS} ip a a 10.100.${NUM}.$((100+SUBNUM))/16 dev eth0
}


ip netns add ${NS}
ip netns exec ${NS} ip link set lo up
ip link add veth1-${NUM} type veth peer name veth2-${NUM}
ip link set veth2-${NUM} netns ${NS}

ip link set veth1-${NUM} master br-evpn
ip link set veth1-${NUM} up
ip netns exec ${NS} ip link set veth2-${NUM} name eth0
ip netns exec ${NS} ip addr add 10.0.0.$((100 + NUM))/24 dev eth0

ip netns exec ${NS} ip link add name br0 type bridge
ip netns exec ${NS} ip link add vxlan1 type vxlan id 0 dev eth0 dstport 4789
ip netns exec ${NS} ip link set vxlan1 master br0
ip netns exec ${NS} ip link set eth0 up
ip netns exec ${NS} ip link set br0 up
ip netns exec ${NS} ip link set vxlan1 up

create_subns 1
create_subns 2
create_subns 3


# Start gobgp
sed "s/__NUM__/$NUM/g" gobgp-host.conf > gobgp-host${NUM}.conf
nohup ip netns exec evpn-${NUM} ./gobgpd -f gobgp-host${NUM}.conf > ./logs/gobgp-host${NUM}.log 2>&1 &

sleep 0.5
# Advertise VTEP
ip netns exec evpn-${NUM} ./gobgp global rib -a evpn add prefix 10.0.0.$((100 + NUM))/32 etag 1 rd $((65000 + NUM)):100
