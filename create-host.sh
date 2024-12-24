#!/bin/bash
set -x
set -e

NUM=$1
NS=evpn-${NUM}


# Create the netns. This is a simulator of a hardware host (a hypervisor) running VMs
ip netns add ${NS}
ip netns exec ${NS} ip link set lo up
ip link add veth1-${NUM} type veth peer name veth2-${NUM}  # to connect the "host" on the simulated hardware switch
ip link set veth2-${NUM} netns ${NS}
ip link set veth1-${NUM} master br-evpn
ip link set veth1-${NUM} up
ip netns exec ${NS} ip link set veth2-${NUM} name eth0
ip netns exec ${NS} ip addr add 10.0.0.$((100 + NUM))/24 dev eth0


# Create bridge and enable vlans by setting vlan_filtering = 1
# We also create the vxlan interface. Will add vlans to these as we add endpoints (simulated VMs)
ip netns exec ${NS} ip link add name br0 type bridge vlan_filtering 1
ip netns exec ${NS} ip link add vxlan1 type vxlan dev eth0 dstport 4789 external  # Very important to use "external"
ip netns exec ${NS} ip link set vxlan1 master br0
ip netns exec ${NS} ip link set dev vxlan1 type bridge_slave vlan_tunnel on
ip netns exec ${NS} bridge vlan del dev vxlan1 vid 1  # By default, vid 1 will exist, take it out
ip netns exec ${NS} ip link set eth0 up
ip netns exec ${NS} ip link set br0 up
ip netns exec ${NS} ip link set vxlan1 up


# Start gobgp
sed "s/__NUM__/$NUM/g" gobgp-host.conf > gobgp-host${NUM}.conf
nohup ip netns exec evpn-${NUM} ./gobgpd -f gobgp-host${NUM}.conf > ./logs/gobgp-host${NUM}.log 2>&1 &

sleep 0.5
# Advertise VTEP

#TODO: We need to uncomment this to make the host agent run
nohup ip netns exec evpn-${NUM} ./host-agent/host_agent > ./logs/host-agent${NUM}.log 2>&1 &
