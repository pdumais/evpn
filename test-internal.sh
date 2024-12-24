#!/bin/bash
set -e

NUM=$1
SUBCOUNT=$2

echo "Testing that endpoints on same host can ping each other"
for i in $(seq 1 $SUBCOUNT); do
    SUBNS=evpn-e${NUM}${i}
    for n in $(seq 1 $SUBCOUNT); do
        if [ "$i" -eq "$n" ]; then
            continue
        fi
        IP1=10.100.${NUM}.$((100+i))
        IP2=10.100.${NUM}.$((100+n))
        echo -n "Ping $IP2 from $IP1  "
        ip netns exec ${SUBNS} ping -c1 -w2 -q ${IP2} > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo -n "Pass"
        else
            echo -n "FAIL"
        fi
        echo

    done
done

