#!/bin/bash

NUM=$1
NS=evpn-${NUM}

function destroy_subns() {
    SUBNUM=$1
    SUBNS=evpn-e${NUM}${SUBNUM}
    ip netns exec ${SUBNS} ip link del eth0
    ip netns del ${SUBNS}
}

ip netns exec ${NS} ip link del br0
ip link del veth1-${NUM}
ip netns del ${NS}

SUBCOUNT=$(ip netns | grep evpn-e${NUM} | wc -l)
for i in $(seq 1 $SUBCOUNT); do
    destroy_subns $i
done



