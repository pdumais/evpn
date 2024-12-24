all: build rebuild-lab

SHELL := sudo /bin/bash

#
# Create
#
#

%.create-hosts:
	./create-host.sh $*

create-vms:
	./create-vm.sh 1 1 10.100.1.101/16 22001
	./create-vm.sh 1 2 10.100.1.102/16 22001
	./create-vm.sh 1 3 10.100.1.103/16 22001
	./create-vm.sh 2 1 10.100.2.101/16 22001
	./create-vm.sh 2 2 10.100.2.102/16 22001
	./create-vm.sh 2 3 10.100.2.103/16 22001
	./create-vm.sh 3 1 10.100.3.101/16 22001
	./create-vm.sh 3 2 10.100.3.102/16 22001
	./create-vm.sh 3 3 10.100.3.103/16 22001
	./create-vm.sh 1 4 10.200.1.101/24 22002
	./create-vm.sh 1 5 10.200.1.102/24 22002 # 1.102 should be able to ping 1.101 without exiting host
	./create-vm.sh 1 6 10.200.2.101/24 22003
	./create-vm.sh 2 4 10.200.1.103/24 22002  # 101 should be able to ping 103 across hosts with VNI 22002
	./create-vm.sh 2 5 10.200.1.104/24 22003  # This is the 10.200.1.0/24 subnet on VNI 22003. 10.200.1.101 should NOT be able to ping
	./create-vm.sh 2 6 10.200.2.102/24 22003  # 2.101 should be able to ping 2.102 across hosts using VNI 22003
	./create-vm.sh 3 4 10.200.1.104/24 22002
	./create-vm.sh 3 5 10.200.2.103/24 22003
	./create-vm.sh 3 6 10.100.3.104/16 22002 # This should not be reachable since it is on another vlan



create-host-infra:
	ip link add name br-evpn type bridge
	ip link set br-evpn up

create-controller: 
	ip netns add evpn-controller
	ip link add veth-ctrl1 type veth peer name veth-ctrl2
	ip link set veth-ctrl1 master br-evpn
	ip link set veth-ctrl1 up
	ip link set veth-ctrl2 netns evpn-controller
	ip netns exec evpn-controller ip link set veth-ctrl2 name eth0
	ip netns exec evpn-controller ip a a 10.0.0.10/24 dev eth0
	ip netns exec evpn-controller ip link set eth0 up
	ip netns exec evpn-controller ip link set lo up
	mkdir -p logs
	nohup ip netns exec evpn-controller ./gobgpd -f gobgp-controller.conf > ./logs/gobgpd.log 2>&1 &
	

#
# Destroy
#
#
%.destroy-hosts:
	./destroy-host.sh $*

destroy-controller:
	- ip link del veth-ctrl1
	- ip netns del evpn-controller

kill-bgp:
	- killall gobgpd	
	- killall host_agent

#
# Main commands
#
#
build:
	cd host-agent && make

create-lab: create-host-infra create-controller 1.create-hosts 2.create-hosts 3.create-hosts create-vms
destroy-lab: kill-bgp destroy-controller 1.destroy-hosts 2.destroy-hosts 3.destroy-hosts
	- ip link del br-evpn

rebuild-lab: destroy-lab create-lab


# This is just to test. But we want this to be done by the BGP server
manual-fdb:
	ip netns exec evpn-1 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.102 dev vxlan1 src_vni 22001
	ip netns exec evpn-1 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.103 dev vxlan1 src_vni 22001
	ip netns exec evpn-2 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.101 dev vxlan1 src_vni 22001
	ip netns exec evpn-2 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.103 dev vxlan1 src_vni 22001
	ip netns exec evpn-3 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.101 dev vxlan1 src_vni 22001
	ip netns exec evpn-3 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.102 dev vxlan1 src_vni 22001
	ip netns exec evpn-1 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.102 dev vxlan1 src_vni 22002
	ip netns exec evpn-1 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.103 dev vxlan1 src_vni 22002
	ip netns exec evpn-2 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.101 dev vxlan1 src_vni 22002
	ip netns exec evpn-2 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.103 dev vxlan1 src_vni 22002
	ip netns exec evpn-3 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.101 dev vxlan1 src_vni 22002
	ip netns exec evpn-3 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.102 dev vxlan1 src_vni 22002
	ip netns exec evpn-1 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.102 dev vxlan1 src_vni 22003
	ip netns exec evpn-1 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.103 dev vxlan1 src_vni 22003
	ip netns exec evpn-2 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.101 dev vxlan1 src_vni 22003
	ip netns exec evpn-2 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.103 dev vxlan1 src_vni 22003
	ip netns exec evpn-3 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.101 dev vxlan1 src_vni 22003
	ip netns exec evpn-3 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.102 dev vxlan1 src_vni 22003

test:
	./test-internal.sh 1 3 
	./test-internal.sh 2 3 
	./test-internal.sh 3 3
	ip netns exec evpn-e14 ping -c1 -w2 -q 10.200.1.102 > /dev/null 2>&1
	ip netns exec evpn-e14 ping -c1 -w2 -q 10.200.1.103 > /dev/null 2>&1
	@if ip netns exec evpn-e14 ping -c1 -w1 -q 10.100.3.104 > /dev/null 2>&1; then exit 1; fi
	ip netns exec evpn-e16 ping -c1 -w2 -q 10.200.2.102 > /dev/null 2>&1
	ip netns exec evpn-e16 ping -c1 -w2 -q 10.200.2.103 > /dev/null 2>&1
	@if ip netns exec evpn-e33 ping -c1 -w1 -q 10.100.3.104 > /dev/null 2>&1; then exit 1; fi
	 

	
install-gobgp:
	curl -L https://github.com/osrg/gobgp/releases/download/v3.32.0/gobgp_3.32.0_linux_amd64.tar.gz -O
	mkdir -p tmp
	mv ./gobgp*.tar.gz tmp/gobgp.tar.gz
	cd tmp; tar -zxf gobgp.tar.gz
	cp tmp/gobgp ./
	cp tmp/gobgpd ./


tail-logs:
	- tmux kill-session -t logsession
	tmux new-session -d -s logsession "tail -F ./logs/gobgpd.log"
	tmux split-window -t logsession -v "watch -t ip netns exec evpn-controller ./gobgp neighbor"
	tmux split-window -t logsession -h  "watch -t ip netns exec evpn-1 ./gobgp global rib -a evpn"
	tmux split-window -t logsession -v "watch -t ip netns exec evpn-2 ./gobgp global rib -a evpn"
	tmux split-window -t logsession -v "watch -t ip netns exec evpn-3 ./gobgp global rib -a evpn"
	tmux split-window -t logsession:0.1 -v "watch -t ip netns exec evpn-controller ./gobgp global rib -a evpn"
	
	#tmux resize-pane -t logsession:0.1 -y 70
	tmux attach-session -t logsession