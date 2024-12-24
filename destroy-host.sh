#!/bin/bash

NUM=$1
NS=evpn-${NUM}

function destroy_subns() {
    SUBNS=$1
    ip netns exec ${SUBNS} ip link del eth0
    ip netns del ${SUBNS}
}

ip netns exec ${NS} ip link del br0
ip link del veth1-${NUM}
ip netns del ${NS}

for n in $(ip netns | grep evpn-e${NUM}); do
    destroy_subns $n
done



